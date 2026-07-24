#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <cerrno>
#include <emmintrin.h>
#include <algorithm>
#include <mutex>
#include <iomanip>

#include "../src/protocol/order.hpp"
#include "../src/protocol/response.hpp"

uint64_t nowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void run_c10k_trader(int thread_id, int num_connections, int orders_per_conn, std::vector<uint64_t>& all_latencies, std::mutex& mtx) {
    std::vector<int> sockets;
    sockets.reserve(num_connections);

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    // 1. Establish all connections for this thread
    for (int i = 0; i < num_connections; i++) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) continue;

        if (connect(sockfd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == 0) {
            int opt = 1;
            setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
            set_nonblocking(sockfd); 
            sockets.push_back(sockfd);
        } else {
            close(sockfd);
        }
    }

    std::cout << "[Thread " << thread_id << "] Connected " << sockets.size() << " concurrent sockets.\n";

    if (sockets.empty()) {
        std::cerr << "[Thread " << thread_id << "] No sockets connected. Bailing out.\n";
        return;
    }

    std::vector<uint64_t> local_latencies;
    local_latencies.reserve(orders_per_conn);

    Order order{0, 0, 1001, 100, 19050, Side::Buy};
    GatewayResponse response{};

    // 2. Blast the server asynchronously
    for (int i = 0; i < orders_per_conn; i++) {
        uint64_t send_time = nowNs();
        order.timestampNs = send_time;

        for (int fd : sockets) {
            order.orderId = fd * 100000 + i;
            
            while (true) {
                ssize_t s = send(fd, &order, sizeof(order), 0);
                if (s >= 0) break; 
                if (errno != EAGAIN && errno != EWOULDBLOCK) break; 
                _mm_pause();
            }
        }

        // Spin-read until EVERY socket gets its response back
        for (int fd : sockets) {
            while (recv(fd, &response, sizeof(response), 0) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    _mm_pause(); 
                } else {
                    break; 
                }
            }
        }
        
        // Record the average round-trip time for this specific batch of orders
        uint64_t batch_rtt = nowNs() - send_time;
        local_latencies.push_back(batch_rtt / sockets.size());
    }

    // Safely merge this thread's latencies into the global list
    std::lock_guard<std::mutex> lock(mtx);
    all_latencies.insert(all_latencies.end(), local_latencies.begin(), local_latencies.end());
    
    for (int fd : sockets) {
        close(fd);
    }
}

int main() {
    constexpr int NUM_THREADS = 4;       
    constexpr int CONNS_PER_THREAD = 2500; // 10,000 Total Connections
    
    // --- REDUCED FOR 2-MINUTE ENDURANCE RUN ---
    constexpr int ORDERS_PER_CONN = 1200;  // 12,000,000 Total Orders
    
    constexpr int TOTAL_ORDERS = NUM_THREADS * CONNS_PER_THREAD * ORDERS_PER_CONN;

    std::cout << "--- Starting C10K Massive Concurrency Endurance Benchmark ---\n";
    std::cout << "Threads      : " << NUM_THREADS << "\n";
    std::cout << "Connections  : " << (NUM_THREADS * CONNS_PER_THREAD) << "\n";
    std::cout << "Total Orders : " << TOTAL_ORDERS << "\n";
    std::cout << "Connecting sockets to server (this takes a second)...\n";
    std::cout << "Running endurance test (Expected time: ~2 minutes)...\n";

    std::vector<uint64_t> all_latencies;
    std::mutex latencies_mtx;
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(run_c10k_trader, i + 1, CONNS_PER_THREAD, ORDERS_PER_CONN, std::ref(all_latencies), std::ref(latencies_mtx));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(end - start).count();

    if (all_latencies.empty()) {
        std::cerr << "No latencies recorded. Benchmark failed.\n";
        return 1;
    }

    // Sort the latencies to calculate percentiles
    std::sort(all_latencies.begin(), all_latencies.end());

    uint64_t p50 = all_latencies[all_latencies.size() * 0.50];
    uint64_t p90 = all_latencies[all_latencies.size() * 0.90];
    uint64_t p99 = all_latencies[all_latencies.size() * 0.99];
    uint64_t p99_9 = all_latencies[all_latencies.size() * 0.999];
    
    // --- ADDED MAX LATENCY ---
    uint64_t max_lat = all_latencies.back(); 

    // Calculate total average
    uint64_t total_sum = 0;
    for (uint64_t lat : all_latencies) total_sum += lat;
    uint64_t avg = total_sum / all_latencies.size();

    std::cout << "\n============================================\n";
    std::cout << "            BENCHMARK RESULTS               \n";
    std::cout << "============================================\n";
    std::cout << std::left << std::setw(20) << "Total Time:" << elapsed << " seconds\n";
    std::cout << std::left << std::setw(20) << "Throughput:" << static_cast<int>(TOTAL_ORDERS / elapsed) << " orders/sec\n\n";
    
    std::cout << "--- Round Trip Latency (RTT) Percentiles ---\n";
    std::cout << std::left << std::setw(20) << "Average:" << avg << " ns\n";
    std::cout << std::left << std::setw(20) << "p50 (Median):" << p50 << " ns\n";
    std::cout << std::left << std::setw(20) << "p90:" << p90 << " ns\n";
    std::cout << std::left << std::setw(20) << "p99:" << p99 << " ns\n";
    std::cout << std::left << std::setw(20) << "p99.9 (Tail):" << p99_9 << " ns\n";
    std::cout << std::left << std::setw(20) << "Max Latency:" << max_lat << " ns\n";
    std::cout << "============================================\n";

    return 0;
}