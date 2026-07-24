#include "strategy.hpp"
#include <iostream>
#include <emmintrin.h> // For _mm_pause()

void StrategyEngine::run(SPSCQueue<Order>& inbound_queue) {
    std::cout << "[Strategy] Engine thread started. Spinning for orders...\n";
    
    Order incoming_order{};
    uint64_t executed_count = 0;

    while (true) {
        if (inbound_queue.pop(incoming_order)) {
            // In a real system, you calculate alpha, check risk, and fire an outbound order here.
            executed_count++;

            // We only print every 100,000 orders so we don't slow down the CPU with I/O bottlenecks.
            if (executed_count % 100000 == 0) {
                std::cout << "[Strategy] Processed " << executed_count 
                          << " orders (Last Order ID: " << incoming_order.orderId << ")\n";
            }
        } else {
            // Hardware Hint: Queue is empty, yield CPU resources briefly
            _mm_pause();
        }
    }
}