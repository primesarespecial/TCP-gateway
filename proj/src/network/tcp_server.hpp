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
    
    // We use the file descriptor (int) as the index to achieve O(1) array lookups.
    // This is vastly faster than a std::map or hash table.
    std::vector<std::unique_ptr<Connection>> connections_;
};