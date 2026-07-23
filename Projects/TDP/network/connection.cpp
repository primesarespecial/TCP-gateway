#include "connection.hpp"

Connection::Connection(int fd)
    : fd_(fd)
{
    // Initialize the static requests to point to themselves and this connection
    recvReq.type = IoRequest::Type::Receive;
    recvReq.connection = this;
    
    sendReq.type = IoRequest::Type::Send;
    sendReq.connection = this;
}

int Connection::fd() const
{
    return fd_;
}