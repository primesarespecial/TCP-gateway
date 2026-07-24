#pragma once

#include <vector>
#include <memory>
#include "connection.hpp"

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    bool start();
    int getListeningFd() const { return listenFd_; }
    
    void addConnection(int clientFd);
    Connection* getConnection(int clientFd);

private:
    int port_;
    int listenFd_{-1};
    
    std::vector<std::unique_ptr<Connection>> connections_;
};
