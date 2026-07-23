#include "latency_tracker.hpp"

#include <algorithm>
#include <iostream>

void LatencyTracker::add(uint64_t latency)
{
    samples_.push_back(latency);
}

void LatencyTracker::print()
{
    if(samples_.empty())
        return;

    std::sort(
        samples_.begin(),
        samples_.end()
    );

    auto percentile =
        [&](double p)
        {
            size_t idx =
                static_cast<size_t>(
                    p * (samples_.size() - 1)
                );

            return samples_[idx];
        };

    uint64_t total = 0;

    for(auto x : samples_)
        total += x;

    std::cout
        << "\n========== LATENCY ==========\n";

    std::cout
        << "Samples : "
        << samples_.size()
        << "\n";

    std::cout
        << "Average : "
        << total / samples_.size()
        << " ns\n";

    std::cout
        << "Median  : "
        << percentile(0.50)
        << " ns\n";

    std::cout
        << "P90     : "
        << percentile(0.90)
        << " ns\n";

    std::cout
        << "P95     : "
        << percentile(0.95)
        << " ns\n";

    std::cout
        << "P99     : "
        << percentile(0.99)
        << " ns\n";

    std::cout
        << "Max     : "
        << samples_.back()
        << " ns\n";

    std::cout
        << "=============================\n";
}