#include "connection.hpp"
#include <unistd.h>

Connection::Connection(int fd) 
    : fd_(fd), 
      recvReq{IoRequest::Type::Receive, this}, 
      sendReq{IoRequest::Type::Send, this} 
{
}

Connection::~Connection() {
    if (fd_ >= 0) {
        close(fd_);
    }
}