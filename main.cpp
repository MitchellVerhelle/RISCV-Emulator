#include "hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include <iostream>

using rv::MemoryBus;
using rv::HashTable;
using rv::Cache;
using rv::RiscV;

int main()
{
    auto dram = std::unique_ptr<MemoryBus>{
        std::make_unique<HashTable<std::uint32_t,std::uint32_t>>() };

    auto l1   = std::make_unique<Cache>(64, 2, std::move(dram));

    RiscV cpu{*l1};

    /* assembled:
       0x00000000: addi x1,x0,5
       0x00000004: addi x2,x1,7
       0x00000008: sw   x2,16(x0)    ; <‑‑ store result after the code
       0x0000000c: jalr x0,x0,0      ; halt (jump to self)
    */
    constexpr std::uint32_t prog[] = {
        0x00500093,        // addi x1, x0, 5
        0x00708113,        // addi x2, x1, 7
        0x00202823,        // sw   x2, 16(x0)
        0x00000067         // jalr x0, x0, 0
    };
    for (std::size_t i = 0; i < 4; ++i)
        l1->store_word(static_cast<std::uint32_t>(i * 4), prog[i]);

    for (int i = 0; i < 4; ++i) cpu.step();

    std::cout << "x2 = "       << cpu.reg(2)              << '\n'
              << "DRAM[16] = " << l1->load_word(16).value_or(0) << '\n'
              << l1->stats().pretty() << '\n';
}
