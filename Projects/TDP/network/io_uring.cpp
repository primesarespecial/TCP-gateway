#include "io_uring.hpp"
#include <iostream>

IoUring::IoUring() = default;

IoUring::~IoUring()
{
    io_uring_queue_exit(&ring_);
}

bool IoUring::initialize(unsigned entries)
{
    int ret = io_uring_queue_init(entries, &ring_, 0);

    if (ret < 0)
    {
        std::cerr << "io_uring_queue_init failed\n";
        return false;
    }

    return true;
}

bool IoUring::submitAccept(int listenFd, IoRequest* acceptReq)
{
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) return false;

    client_len_ = sizeof(client_addr_);
    io_uring_prep_accept(
        sqe,
        listenFd,
        reinterpret_cast<sockaddr*>(&client_addr_),
        &client_len_,
        0
    );
    io_uring_sqe_set_data(sqe, acceptReq);
    return io_uring_submit(&ring_) > 0;
}

bool IoUring::submitRecv(Connection* conn)
{
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) return false;

    io_uring_prep_recv(
        sqe,
        conn->fd(),
        &conn->orderBuffer,
        sizeof(Order),
        0
    );
    io_uring_sqe_set_data(sqe, &conn->recvReq);
    return io_uring_submit(&ring_) > 0;
}

bool IoUring::submitSend(Connection* conn)
{
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) return false;

    io_uring_prep_send(
        sqe,
        conn->fd(),
        &conn->responseBuffer,
        sizeof(GatewayResponse),
        0
    );
    io_uring_sqe_set_data(sqe, &conn->sendReq);
    return io_uring_submit(&ring_) > 0;
}

IoCompletion IoUring::waitCompletion()
{
    io_uring_cqe* cqe = nullptr;
    int ret = io_uring_wait_cqe(&ring_, &cqe);

    if (ret < 0) return { nullptr, ret };

    auto* req = static_cast<IoRequest*>(io_uring_cqe_get_data(cqe));
    IoCompletion completion{ req, cqe->res };
    io_uring_cqe_seen(&ring_, cqe);

    return completion;
}

IoCompletion IoUring::peekCompletion()
{
    io_uring_cqe* cqe = nullptr;
    int ret = io_uring_peek_cqe(&ring_, &cqe);

    // If ret < 0, it usually means -EAGAIN (no more events ready)
    if (ret < 0) return { nullptr, ret };

    auto* req = static_cast<IoRequest*>(io_uring_cqe_get_data(cqe));
    IoCompletion completion{ req, cqe->res };
    io_uring_cqe_seen(&ring_, cqe);

    return completion;
}