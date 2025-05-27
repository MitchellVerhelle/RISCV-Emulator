#include "concurrent_hash_table.hpp"
#include <thread>
#include <vector>
#include <random>
#include <iostream>

using Table = rv::ConcurrentHashTable<std::uint32_t,std::uint32_t>;

int main()
{
    constexpr std::size_t n_threads = 8;
    constexpr std::size_t ops_per_t = 200'000;

    Table tbl(1 << 16);            // 65 536 buckets

    auto worker = [&](unsigned id){
        std::mt19937 rng(id * 17u);
        std::uniform_int_distribution<std::uint32_t> dist(0, 1'000'000);

        for (std::size_t i = 0; i < ops_per_t; ++i) {
            auto k = dist(rng);
            tbl.put(k, k + 1);
            (void)tbl.get(k);
        }
    };

    std::vector<std::thread> pool;
    for (unsigned i = 0; i < n_threads; ++i)
        pool.emplace_back(worker, i);

    for (auto& t : pool) t.join();

    std::cout << "Concurrent test finished.\n"
              << "Table size  : " << tbl.size() << '\n';
}
