#pragma once
#include <cstdint>

enum class ResponseType : uint8_t {
    Accepted = 0,
    Rejected = 1
};

struct [[gnu::packed]] GatewayResponse {
    ResponseType type;
    uint64_t orderId;
};