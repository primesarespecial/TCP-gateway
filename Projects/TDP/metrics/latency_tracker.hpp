#pragma once

#include <cstdint>
#include <vector>

class LatencyTracker
{
public:

    void add(uint64_t latency);

    void print();

private:

    std::vector<uint64_t> samples_;
};