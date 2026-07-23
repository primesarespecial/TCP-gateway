#pragma once

#include <cstdint>
#include <unordered_map>

#include "connection.hpp"

class TcpServer
{
public:

    explicit TcpServer(uint16_t port);

    ~TcpServer();

    bool start();

    int getListeningFd() const;


    void addConnection(int fd);

    Connection* getConnection(int fd);

private:

    int listen_fd_;

    uint16_t port_;
    

    std::unordered_map<int, Connection> connections_;
};