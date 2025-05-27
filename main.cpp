#include "concurrent_hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include "rv_assembler.hpp"
#include "cache_stats_formatter.hpp"
#include <cassert>
#include <format>
#include <iostream>
#include <exception>
#ifndef __EMSCRIPTEN__
#  include <oneapi/dpl/execution>
#  include <oneapi/dpl/algorithm>
#  include <oneapi/dpl/iterator>
#else
#  include <algorithm> // std::for_each fallback
#endif
#include <chrono>

using rv::Cache;
using rv::ConcurrentHashTable;
using rv::RiscV;
using namespace std::chrono;
int main()
{
    auto dram = std::make_unique<ConcurrentHashTable<std::uint32_t,std::uint32_t>>();
    auto l1   = std::make_unique<Cache>(64, 2, std::move(dram));
    RiscV cpu{ *l1 };

    // load program
    constexpr std::string_view asm_src = R"(
start:
    addi x1, x0, 11       # loop upper-bound (exclusive)
    addi x2, x0, 0        # sum
    addi x3, x0, 1        # i = 1
loop:
    add  x2, x2, x3
    addi x3, x3, 1
    bne  x3, x1, loop
    sw   x2, 32(x0)
    jalr x0, x0, 0        # halt
)";

    auto words = rv::assemble(asm_src); // assemble the program (throws on error)
    std::uint32_t base = 0;
    auto t_load_start = high_resolution_clock::now();

#ifdef __EMSCRIPTEN__
    for (std::size_t i = 0; i < words.size(); ++i) {
        l1->store_word(base + static_cast<std::uint32_t>(i * 4), words[i]);
    }
#else
    auto first = oneapi::dpl::counting_iterator<std::size_t>(0);
    auto last  = first + static_cast<std::uint32_t>(words.size());
    oneapi::dpl::for_each(oneapi::dpl::execution::par_unseq,
        first, last,
        [&](std::size_t i) {
            l1->store_word(base + static_cast<std::uint32_t>(i * 4), words[i]);
        });
#endif
    
    auto t_load_end = high_resolution_clock::now();
    auto load_ms = duration_cast<microseconds>(t_load_end - t_load_start).count();
    std::cout << std::format("Load time      : {} us\n", load_ms); // 963 us
    
    
    constexpr std::uint32_t expected = 55; // 1 + ... + 10 = 55
    auto t_exec_start = high_resolution_clock::now();
    // run program
    while (cpu.pc() != static_cast<std::uint32_t>((words.size() - 1) * 4))
        cpu.step();
    // verify result
    const std::uint32_t sum_reg = cpu.reg(2);
    const std::uint32_t sum_mem = l1->load_word(32).value_or(0);

    auto t_exec_end = high_resolution_clock::now();
    auto exec_ms = duration_cast<microseconds>(t_exec_end - t_exec_start).count();
    std::cout << std::format("Execution time : {} us\n\n", exec_ms); // 3 us

    std::cout << std::format(
        "x2  = {:3} (expected 55)\n"
        "MEM = {:3} (expected 55)\n\n",
        sum_reg, sum_mem);
    
    
    // pretty print cache stats
    std::cout << std::format("Single-line: {}\n", l1->stats());
    std::cout << std::format("\nFull block:\n{:full}", l1->stats());

    assert(sum_reg == expected && sum_mem == expected);
    std::cout << "\nAll tests passed! \n";
    return 0;
}
