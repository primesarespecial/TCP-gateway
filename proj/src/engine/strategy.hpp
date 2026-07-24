#pragma once

#include "../../include/spsc_queue.hpp"
#include "../protocol/order.hpp"

class StrategyEngine {
public:
    // Runs on a dedicated CPU core
    void run(SPSCQueue<Order>& inbound_queue);
};