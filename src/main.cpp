#include "hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include <iostream>

int main()
{
    using namespace rv;

    auto dram   = std::make_unique<HashTable<std::uint32_t,std::uint32_t>>();
    auto l1d    = std::make_unique<Cache>(64, 2, std::move(dram));
    RiscV cpu{*l1d};

    /* tiny program: addi x1, x0, 5 ; addi x2, x1, 7 ; sw x2,0(x0) ; jalr x0,x0,0 */
    std::uint32_t prog[] = { 0x00500093, 0x00708113, 0x00212023, 0x00000067 };
    for (std::size_t i=0;i<4;++i) l1d->store_word(i*4, prog[i]);

    for (int i=0;i<4;++i) cpu.step();
    std::cout << "x2 = " << cpu.reg(2) << '\n';
    std::cout << "DRAM[0] = "
              << l1d->load_word(0).value_or(0) << '\n';
    std::cout << l1d->stats().pretty() << '\n';
}
