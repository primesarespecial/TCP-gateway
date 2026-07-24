#pragma once

#include "io_request.hpp"
#include "../protocol/order.hpp"
#include "../protocol/response.hpp"

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();

    int fd() const { return fd_; }

    // State machine requests
    IoRequest recvReq;
    IoRequest sendReq;

    // Pre-allocated buffers for zero-copy parsing
    Order orderBuffer{};
    GatewayResponse responseBuffer{};

private:
    int fd_;
};