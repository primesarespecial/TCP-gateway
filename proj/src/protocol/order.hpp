#pragma once
#include <cstdint>

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

struct [[gnu::packed]] Order {
    uint64_t orderId;
    uint64_t timestampNs;
    uint32_t symbol;
    uint32_t quantity;
    uint32_t price;
    Side side;
};