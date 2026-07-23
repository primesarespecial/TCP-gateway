#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "../protocol/order.hpp"
#include "../protocol/response.hpp"
#include <chrono>


uint64_t nowNs()
{
    using namespace std::chrono;

    return duration_cast<nanoseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in server{};

    server.sin_family = AF_INET;
    server.sin_port = htons(9000);

    if (inet_pton(
            AF_INET,
            "127.0.0.1",
            &server.sin_addr) != 1)
    {
        perror("inet_pton");
        return 1;
    }

    if (connect(
            sockfd,
            reinterpret_cast<sockaddr*>(&server),
            sizeof(server)) < 0)
    {
        perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to TDP\n";

     Order order{
    1,
    nowNs(),
    1001,
    100,
    19050,
    Side::Buy
    };
    ssize_t sent = send(
        sockfd,
        &order,
        sizeof(order),
        0
    );
    GatewayResponse response{};

int n = recv(
    sockfd,
    &response,
    sizeof(response),
    0
);

if(n == sizeof(GatewayResponse))
{
    if(response.type == ResponseType::Accepted)
    {
        std::cout
            << "Order Accepted\n";
    }
    else
    {
        std::cout
            << "Order Rejected\n";
    }
}

    if (sent < 0)
    {
        perror("send");
    }
    else
    {
        std::cout
            << "Sent "
            << sent
            << " bytes\n";
    }

    close(sockfd);

    return 0;
}