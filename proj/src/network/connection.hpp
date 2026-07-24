#pragma once

#include "io_request.hpp"
#include "../protocol/order.hpp"
#include "../protocol/response.hpp"

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();

    int fd() const { return fd_; }

    IoRequest recvReq;
    IoRequest sendReq;

    Order orderBuffer{};
    GatewayResponse responseBuffer{};

private:
    int fd_;
};
