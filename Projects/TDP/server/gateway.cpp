#include <chrono>
#include <iostream>
#include <unistd.h>
#include <netinet/tcp.h>

#include "../network/socket.hpp"
#include "../network/connection.hpp"
#include "../network/io_uring.hpp"
#include "../network/io_request.hpp"
#include "../protocol/order.hpp"
#include "../protocol/response.hpp"
#include "../validation/validator.hpp"
#include "../metrics/latency_tracker.hpp"

#define DEBUG_OUTPUT 0

uint64_t nowNs()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

int main()
{
    TcpServer server(9000);
    if (!server.start()) return 1;

    IoUring ring;
    
    // MASSIVE queue depth to absorb the 1 million order spikes
    if (!ring.initialize(8192)) return 1;

    IoRequest acceptReq{IoRequest::Type::Accept, nullptr};

    if (!ring.submitAccept(server.getListeningFd(), &acceptReq))
    {
        std::cerr << "Failed to submit initial accept\n";
        return 1;
    }

    Validator validator;
    LatencyTracker tracker;

    uint64_t totalLatency = 0;
    uint64_t ordersProcessed = 0;

    std::cout << "TDP Async Gateway running (Batched Reaping Enabled)...\n";

    while (true)
    {
        // 1. Block and wait for AT LEAST one event to hit the kernel
        IoCompletion completion = ring.waitCompletion();

        // 2. Process ALL available events currently in memory without making another syscall
        while (completion.request != nullptr)
        {
            IoRequest* req = completion.request;
            int result = completion.result;

            if (result < 0)
            {
                if (req->connection) close(req->connection->fd());
            }
            else
            {
                switch (req->type)
                {
                    case IoRequest::Type::Accept:
                    {
                        int clientFd = result;
                        int opt = 1;
                        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

                        server.addConnection(clientFd);
                        Connection* conn = server.getConnection(clientFd);

                        ring.submitRecv(conn);
                        ring.submitAccept(server.getListeningFd(), &acceptReq);
                        break;
                    }

                    case IoRequest::Type::Receive:
                    {
                        if (result == 0 || result != sizeof(Order))
                        {
                            close(req->connection->fd());
                            break;
                        }

                        Connection* conn = req->connection;
                        uint64_t latency = nowNs() - conn->orderBuffer.timestampNs;
                        
                        tracker.add(latency);
                        totalLatency += latency;
                        ordersProcessed++;

                        bool valid = validator.validate(conn->orderBuffer);

                        conn->responseBuffer.orderId = conn->orderBuffer.orderId;
                        conn->responseBuffer.type = valid ? ResponseType::Accepted : ResponseType::Rejected;

                        ring.submitSend(conn);

                        if (ordersProcessed % 50000 == 0)
                        {
                            std::cout << "\n=========== TDP ===========\n";
                            std::cout << "Orders Processed : " << ordersProcessed << "\n";
                            std::cout << "Average Latency  : " << totalLatency / ordersProcessed << " ns\n";
                            tracker.print();
                            std::cout << "===========================\n";
                        }
                        break;
                    }

                    case IoRequest::Type::Send:
                    {
                        ring.submitRecv(req->connection);
                        break;
                    }
                }
            }

            // Peek the ring buffer to grab the next event instantly
            completion = ring.peekCompletion();
        }
    }

    return 0;
}