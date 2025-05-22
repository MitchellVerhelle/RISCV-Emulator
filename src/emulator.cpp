#include "mmio_window.hpp"
#include "hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include <memory>

namespace rv {

static std::unique_ptr<HashTable<std::uint32_t,std::uint32_t>> dram_up;
static std::unique_ptr<MmioWindow>                             mmio_up;
static std::unique_ptr<Cache>                                  cache_up;
static std::unique_ptr<RiscV>                                  cpu_up;

void build_system(MmioWindow*& io_out, RiscV*& cpu_out)
{
    dram_up  = std::make_unique<HashTable<std::uint32_t,std::uint32_t>>();

    mmio_up  = std::make_unique<MmioWindow>(std::move(dram_up));
    MmioWindow* io = mmio_up.get();

    cache_up = std::make_unique<Cache>(64, 2, std::move(mmio_up));

    cpu_up   = std::make_unique<RiscV>(*cache_up);

    io_out  = io;
    cpu_out = cpu_up.get();

    static const uint32_t nop = 0x00000013;       // addi x0,x0,0
    static const uint32_t halt = 0x00000067; // jalr x0,x0,0
    io_out->store_word(0, nop);
    io_out->store_word(4, halt);
}

} // namespace rv