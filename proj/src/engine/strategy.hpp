#pragma once

#include "../../include/spsc_queue.hpp"
#include "../protocol/order.hpp"

class StrategyEngine {
public:
   
    void run(SPSCQueue<Order>& inbound_queue);
};
