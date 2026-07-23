#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

#include "../protocol/order.hpp"
#include "../protocol/response.hpp"

uint64_t nowNs()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

void run_trader(int trader_id, int num_orders, std::atomic<uint64_t>& total_latency)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return;

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        close(sockfd);
        return;
    }

    int opt = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

    uint64_t thread_latency = 0;

    for(int i = 1; i <= num_orders; i++)
    {
        uint64_t send_time = nowNs();
        
        Order order{
            static_cast<uint64_t>(trader_id * 1000000 + i), 
            send_time,
            1001,
            100,
            19050,
            Side::Buy
        };

        if (send(sockfd, &order, sizeof(order), 0) < 0) break;

        GatewayResponse response{};
        if (recv(sockfd, &response, sizeof(response), 0) <= 0) break;

        thread_latency += (nowNs() - send_time);
    }

    total_latency += thread_latency;
    close(sockfd);
}

int main()
{
    // Tuned to 4 threads to prevent OS context-switch thrashing
    constexpr int NUM_THREADS = 4;       
    constexpr int ORDERS_PER_THREAD = 250000; 
    constexpr int TOTAL_ORDERS = NUM_THREADS * ORDERS_PER_THREAD;

    std::cout << "Starting concurrent benchmark...\n";
    std::cout << "Threads      : " << NUM_THREADS << "\n";
    std::cout << "Total Orders : " << TOTAL_ORDERS << "\n";

    std::atomic<uint64_t> total_round_trip_latency{0};
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back(run_trader, i + 1, ORDERS_PER_THREAD, std::ref(total_round_trip_latency));
    }

    for (auto& t : threads)
    {
        t.join();
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n--- Benchmark Results ---\n";
    std::cout << "Time        : " << elapsed << " s\n";
    std::cout << "Throughput  : " << static_cast<int>(TOTAL_ORDERS / elapsed) << " orders/sec\n";
    std::cout << "Avg Latency : " << (total_round_trip_latency / TOTAL_ORDERS) << " ns (Round Trip)\n";

    return 0;
}