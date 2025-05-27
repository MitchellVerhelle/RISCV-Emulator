#include <iostream>
#include <format>
#include "cache_stats.hpp"
#include "cache_stats_formatter.hpp"

int main()
{
    rv::CacheStats stats;
    stats.n_cpu_accesses = 1234;
    stats.n_hits         = 1010;
    stats.n_misses       = 224;
    stats.n_evictions    = 77;

    std::cout << std::format("Single-line summary:\n{}\n\n", stats);

    std::cout << std::format("Full block:\n{:full}", stats);

    return 0;
}
