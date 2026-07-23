#include "socket.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include "connection.hpp"

TcpServer::TcpServer(uint16_t port)
    : listen_fd_(-1),
      port_(port)
{}

TcpServer::~TcpServer()
{
    if (listen_fd_ >= 0)
    {
        close(listen_fd_);
    }
}

bool TcpServer::start(){
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd_ < 0)
    {
        perror("socket");
        return false;
    }

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9000);
    int opt = 1;

    setsockopt(
      listen_fd_,
       SOL_SOCKET,
       SO_REUSEADDR,
       &opt,
       sizeof(opt)
    );

    if (bind(
            listen_fd_,
            reinterpret_cast<sockaddr*>(&server_addr),
            sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(listen_fd_);
        return false;
    }

     if (listen(listen_fd_, 16) < 0)
    {
        perror("listen");
        close(listen_fd_);
        return false;
    }
    return true;
}

int TcpServer::getListeningFd() const
{
    return listen_fd_;
}

void TcpServer::addConnection(int fd)
{
    // Insert the connection and grab the iterator to its final location in memory
    auto [it, inserted] = connections_.emplace(
        fd,
        Connection(fd)
    );

    // Re-wire the pointers to point to the valid heap object, not the dead stack temporary
    Connection& conn = it->second;
    conn.recvReq.connection = &conn;
    conn.sendReq.connection = &conn;
}

Connection* TcpServer::getConnection(int fd)
{
    auto it = connections_.find(fd);

    if(it == connections_.end())
        return nullptr;

    return &it->second;
}