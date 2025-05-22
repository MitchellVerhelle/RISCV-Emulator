#include "hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include <cassert>
#include <format>
#include <iostream>

using rv::Cache;
using rv::HashTable;
using rv::MemoryBus;
using rv::RiscV;

/* ---------------------------------------------------------------
   Assembled test program (text‑book layout)

   addr  instr                       machine‑code
   ----  --------------------------  ------------
   0x00: addi x1, x0, 11             0x00b00093
   0x04: addi x2, x0, 0              0x00000113
   0x08: addi x3, x0, 1              0x00100193
   0x0c: add  x2, x2, x3             0x00310133   ; sum += i
   0x10: addi x3, x3, 1              0x00118193   ; i++
   0x14: bne  x3, x1, -8             0xfe119ce3   ; loop if i != 10
   0x18: sw   x2, 32(x0)             0x02202023   ; store result
   0x1c: jalr x0, x0, 0              0x00000067   ; halt
 ----------------------------------------------------------------*/
constexpr std::uint32_t prog[] = {
    0x00B00093, 0x00000113, 0x00100193, 0x00310133,
    0x00118193, 0xfe119ce3, 0x02202023, 0x00000067
};

int main()
{
    /* -------------------- build memory hierarchy ---------------- */
    auto dram = std::make_unique<HashTable<std::uint32_t, std::uint32_t>>();
    auto l1   = std::make_unique<Cache>(64, 2, std::move(dram));
    RiscV cpu{*l1};

    /* -------------------- load program into DRAM ---------------- */
    for (std::size_t i = 0; i < std::size(prog); ++i)
        l1->store_word(static_cast<std::uint32_t>(i * 4), prog[i]);

    /* run until we hit the self‑loop @0x1c */
    while (cpu.pc() != 0x1c)
        cpu.step();

    /* -------------------- validate results ---------------------- */
    const std::uint32_t expected = 55;              // 1‥10 triangular sum
    const std::uint32_t sum_reg  = cpu.reg(2);
    const std::uint32_t sum_mem  = l1->load_word(32).value_or(0);

    std::cout << std::format(
        "x2  = {:3}   (expected 55)\n"
        "MEM = {:3}   (expected 55)\n{}\n",
        sum_reg, sum_mem, l1->stats().pretty());

    assert(sum_reg == expected && "x2 holds wrong sum");
    assert(sum_mem == expected && "memory[32] holds wrong sum");

    std::cout << "All tests passed ✔︎\n";
    return 0;
}
