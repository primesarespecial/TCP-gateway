#include "strategy.hpp"
#include <iostream>
#include <emmintrin.h> 

void StrategyEngine::run(SPSCQueue<Order>& inbound_queue) {
    std::cout << "[Strategy] Engine thread started. Spinning for orders...\n";
    
    Order incoming_order{};
    uint64_t executed_count = 0;

    while (true) {
        if (inbound_queue.pop(incoming_order)) {
            
            executed_count++;
            
            if (executed_count % 100000 == 0) {
                std::cout << "[Strategy] Processed " << executed_count 
                          << " orders (Last Order ID: " << incoming_order.orderId << ")\n";
            }
        } else {
            _mm_pause();
        }
    }
}
