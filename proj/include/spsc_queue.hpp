#pragma once

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <vector>

constexpr std::size_t CACHE_LINE_SIZE = 64;

template <typename T>
class SPSCQueue
{
public:
    explicit SPSCQueue(std::size_t capacity)
        : capacity_(capacity), mask_(capacity - 1)
    {
        if ((capacity == 0) || ((capacity & (capacity - 1)) != 0)) {
            throw std::invalid_argument("Capacity must be a power of 2");
        }
        buffer_.resize(capacity_);
    }

    bool push(const T& item);
    bool pop(T& item);

private:
    std::size_t capacity_;
    std::size_t mask_;
    std::vector<T> buffer_;

    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail_{0};
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> head_{0};
};

template <typename T>
bool SPSCQueue<T>::push(const T& item)
{
    const std::size_t current_tail = tail_.load(std::memory_order_relaxed);
    const std::size_t current_head = head_.load(std::memory_order_acquire);

    if ((current_tail - current_head) == capacity_) {
        return false; 
    }

    buffer_[current_tail & mask_] = item;
    tail_.store(current_tail + 1, std::memory_order_release);
    return true;
}

template <typename T>
bool SPSCQueue<T>::pop(T& item)
{
    const std::size_t current_head = head_.load(std::memory_order_relaxed);
    const std::size_t current_tail = tail_.load(std::memory_order_acquire);

    if (current_head == current_tail) {
        return false;
    }

    item = buffer_[current_head & mask_];
    head_.store(current_head + 1, std::memory_order_release);
    return true;
}