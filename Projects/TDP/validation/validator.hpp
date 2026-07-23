#pragma once

#include <unordered_set>

#include "../protocol/order.hpp"

class Validator
{
public:
    bool validate(const Order& order);

private:
    std::unordered_set<uint64_t> seenOrders_;
};