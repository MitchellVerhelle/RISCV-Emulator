#pragma once
#include <atomic>
#include <format>
#include <string>
#include <format>

namespace rv {

struct CacheStats
{
    std::atomic<std::uint64_t> n_hits{0};
    std::atomic<std::uint64_t> n_misses{0};
    std::atomic<std::uint64_t> n_evictions{0};
    std::atomic<std::uint64_t> n_cpu_accesses{0};

    [[nodiscard]] double hit_rate()  const noexcept
    { return n_hits ? static_cast<double>(n_hits) / static_cast<double>(n_cpu_accesses) : 0.0; }
    [[nodiscard]] double miss_rate() const noexcept { return 1.0 - hit_rate(); }

    std::string pretty() const
    {
        return std::format("Hits {:8}, Misses {:8}  =>  HR {:5.2f}%",
                           n_hits.load(), n_misses.load(), hit_rate()*100.0);
    }
};

} // namespace rv
