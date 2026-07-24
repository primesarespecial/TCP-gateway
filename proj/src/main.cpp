#include <iostream>
#include <thread>
#include <unistd.h>
#include <netinet/tcp.h>
#include <emmintrin.h>
#include <chrono>
#include <pthread.h> 

#include "network/tcp_server.hpp"
#include "network/io_uring.hpp"
#include "network/connection.hpp"
#include "engine/strategy.hpp"
#include "../include/spsc_queue.hpp"

inline uint64_t nowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}


void pinThreadToCore(pthread_t thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

int main() {
    std::cout << "--- Starting High-Performance io_uring Gateway (Pinned) ---\n";

    
    pinThreadToCore(pthread_self(), 1);
    std::cout << "[Gateway] Network thread pinned to CPU Core 1\n";

    SPSCQueue<Order> inbound_queue(32768);

    StrategyEngine engine;
    std::thread strategy_thread(&StrategyEngine::run, &engine, std::ref(inbound_queue));

    
    pinThreadToCore(strategy_thread.native_handle(), 2);
    std::cout << "[Strategy] Engine thread pinned to CPU Core 2\n";

    TcpServer server(9000);
    if (!server.start()) {
        std::cerr << "Failed to start server.\n";
        return 1;
    }

    IoUring ring;
    if (!ring.initialize(8192)) {
        std::cerr << "Failed to initialize io_uring (do you have sudo privileges for SQPOLL?)\n";
        return 1;
    }

    IoRequest acceptReq{IoRequest::Type::Accept, nullptr};
    if (!ring.submitAccept(server.getListeningFd(), &acceptReq)) {
        std::cerr << "Failed to submit initial accept\n";
        return 1;
    }

    std::cout << "[Gateway] Async io_uring thread is live. Waiting for packets...\n";

    uint64_t total_processing_latency = 0;
    uint64_t processed_count = 0;

    while (true) {
        IoCompletion completion = ring.waitCompletion();

        while (completion.request != nullptr) {
            IoRequest* req = completion.request;
            int result = completion.result;

            if (result < 0) {
                if (req->connection) close(req->connection->fd());
            } else {
                switch (req->type) {
                    case IoRequest::Type::Accept: {
                        int clientFd = result;
                        int opt = 1;
                        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

                        server.addConnection(clientFd);
                        Connection* conn = server.getConnection(clientFd);

                        ring.submitRecv(conn);
                        ring.submitAccept(server.getListeningFd(), &acceptReq);
                        break;
                    }

                    case IoRequest::Type::Receive: {
                        if (result == 0 || result != sizeof(Order)) {
                            close(req->connection->fd());
                            break;
                        }

                        uint64_t start_process = nowNs();
                        Connection* conn = req->connection;

                        while (!inbound_queue.push(conn->orderBuffer)) {
                            _mm_pause(); 
                        }

                        conn->responseBuffer.orderId = conn->orderBuffer.orderId;
                        conn->responseBuffer.type = ResponseType::Accepted;

                        ring.submitSend(conn);

                        uint64_t end_process = nowNs();
                        total_processing_latency += (end_process - start_process);
                        processed_count++;

                        if (processed_count % 100000 == 0) {
                            std::cout << "[Gateway] Avg Internal Processing Latency: " 
                                      << (total_processing_latency / processed_count) << " ns\n";
                        }
                        break;
                    }

                    case IoRequest::Type::Send: {
                        ring.submitRecv(req->connection);
                        break;
                    }
                }
            }
            completion = ring.peekCompletion();
        }
    }

    strategy_thread.join();
    return 0;
}
