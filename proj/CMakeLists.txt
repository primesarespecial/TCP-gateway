cmake_minimum_required(VERSION 3.20)
project(Proj)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# HFT Optimization flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall -Wextra -pthread")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)

include_directories(
    include
    ${LIBURING_INCLUDE_DIRS}
)

# The Main Gateway Executable
add_executable(gateway
    src/main.cpp
    src/network/connection.cpp
    src/network/io_uring.cpp
    src/network/tcp_server.cpp
    src/engine/strategy.cpp
)
target_link_libraries(gateway ${LIBURING_LIBRARIES})

# The Stress Tester
add_executable(benchmark_client
    benchmark/benchmark_client.cpp
)