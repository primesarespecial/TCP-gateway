#pragma once
#include <cstdint>
#include "io_request.hpp"
#include "../protocol/order.hpp"
#include "../protocol/response.hpp"

class Connection
{
public:
    explicit Connection(int fd);

    int fd() const;

    // Pre-allocated components strictly for this connection
    IoRequest recvReq;
    IoRequest sendReq;
    
    Order orderBuffer;
    GatewayResponse responseBuffer;

private:
    int fd_;
};