#pragma once

#include <liburing.h>
#include <netinet/in.h>
#include "connection.hpp"
#include "io_request.hpp"

struct IoCompletion {
    IoRequest* request;
    int result;
};

class IoUring {
public:
    IoUring();
    ~IoUring();

    bool initialize(unsigned entries = 8192);

    bool submitAccept(int listenFd, IoRequest* acceptReq);
    bool submitRecv(Connection* conn);
    bool submitSend(Connection* conn);

    // Blocking wait for the first event
    IoCompletion waitCompletion();
    
    // Non-blocking peek to batch-reap the rest
    IoCompletion peekCompletion(); 

private:
    io_uring ring_;
    sockaddr_in client_addr_{};
    socklen_t client_len_{sizeof(client_addr_)};
};