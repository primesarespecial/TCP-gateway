A low-latency, zero-syscall TCP gateway built in C++ for quantitative trading environments.

Instead of relying on legacy `epoll` multiplexing, this project uses Linux `io_uring` with a dedicated kernel polling thread (`SQPOLL`). This architecture eliminates user-to-kernel context switches on the hot path, achieving constant-time O(1) network scaling.

## Core Architecture

* **io_uring Subsystem:** Uses submission and completion queues (SQ/CQ) to batch network I/O operations asynchronously.

* **Zero-Syscall Dispatch:** Configured with `IORING_SETUP_SQPOLL`. A dedicated kernel thread polls the user-space submission queue, meaning packets are read and dispatched without a single `read()` or `write()` syscall.

* **Lock-Free Concurrency:** Uses a custom Single-Producer Single-Consumer (SPSC) ring buffer. The network thread hands off parsed structs to the strategy engine using hardware-level atomics, avoiding `std::mutex` contention entirely.

* **Mechanical Sympathy:**

  * Threads are pinned to isolated CPU cores (`pthread_setaffinity_np`) to prevent scheduler jitter and maintain L1/L2 cache warmth.

  * Sockets bypass Nagle's algorithm (`TCP_NODELAY`) and operate in strictly non-blocking mode.

## Benchmarks

Stress-tested over the `127.0.0.1` loopback interface. The client and server were isolated to separate CPU cores via `taskset` to prevent L3 cache thrashing.

**Test Conditions:** 10,000 Concurrent TCP Connections | 12,000,000 Total Messages

| **Metric** | **Result** | 
| :--- | :--- |
| **Peak Throughput** | 163,841 msgs/sec | 
| **Internal Handoff Time** | ~245 ns | 

**Round-Trip Tail Latency:**

| **Percentile** | **Latency** | 
| :--- | :--- |
| p50 (Median) | 24.0 µs | 
| p90 | 27.4 µs | 
| p99 | 33.6 µs | 
| p99.9 (Tail) | 40.8 µs | 
| Max | 122.1 µs | 

## Build Instructions

**Requirements:**

* Linux Kernel 5.11+ (Required for advanced `io_uring` features)
* `liburing-dev`
* CMake 3.10+
* GCC/Clang (C++17)

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
