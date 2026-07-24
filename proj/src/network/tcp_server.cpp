#include "tcp_server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

TcpServer::TcpServer(int port) : port_(port) {
    // Pre-allocate space for up to 10,000 concurrent file descriptors
    connections_.resize(20000); 
}

TcpServer::~TcpServer() {
    if (listenFd_ >= 0) {
        close(listenFd_);
    }
}

bool TcpServer::start() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) return false;

    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        return false;
    }

    if (listen(listenFd_, 4096) < 0) {
        return false;
    }

    std::cout << "[Server] Listening on port " << port_ << "\n";
    return true;
}

void TcpServer::addConnection(int clientFd) {
    if (clientFd >= 0 && clientFd < static_cast<int>(connections_.size())) {
        connections_[clientFd] = std::make_unique<Connection>(clientFd);
    }
}

Connection* TcpServer::getConnection(int clientFd) {
    if (clientFd >= 0 && clientFd < static_cast<int>(connections_.size())) {
        return connections_[clientFd].get();
    }
    return nullptr;
}