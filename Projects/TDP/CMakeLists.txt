cmake_minimum_required(VERSION 3.20)

project(TDP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)

include_directories(
    ${LIBURING_INCLUDE_DIRS}
)

add_executable(
    gateway

    server/gateway.cpp
    network/socket.cpp
    network/io_uring.cpp
    network/connection.cpp
    validation/validator.cpp
    metrics/latency_tracker.cpp
)

target_link_libraries(
    gateway
    ${LIBURING_LIBRARIES}
)

add_executable(
    trader

    client/trader_client.cpp
)

target_link_libraries(
    trader
    ${LIBURING_LIBRARIES}
)
add_executable(
    benchmark_client

    benchmark/benchmark_client.cpp
)

target_link_libraries(
    benchmark_client
    ${LIBURING_LIBRARIES}
)