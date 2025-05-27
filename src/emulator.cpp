#include "mmio_window.hpp"
#include "hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include "graphics.hpp"
#include "text/bitmap_font.hpp"
#include <memory>

namespace rv {

static std::unique_ptr<HashTable<std::uint32_t,std::uint32_t>> dram_up;
static std::unique_ptr<Cache>                                  cache_up;
static std::unique_ptr<RiscV>                                  cpu_up;

/* 128×128 RGB332 framebuffer storage */
static std::uint8_t framebuffer[128 * 128]{};    // zero‑initialised

void build_system(MmioWindow*& io_out, RiscV*& cpu_out)
{
    /* bottom‑level DRAM */
    dram_up = std::make_unique<HashTable<std::uint32_t,std::uint32_t>>();

    /* create MMIO window and keep a raw pointer BEFORE std::move */
    auto mmio_ptr = std::make_unique<MmioWindow>(std::move(dram_up));
    mmio_ptr->framebuffer = framebuffer;
    MmioWindow* mmio_raw  = mmio_ptr.get();      // <── raw, stays valid

    /* L1 cache now owns the window */
    cache_up = std::make_unique<Cache>(64, 2, std::move(mmio_ptr));

    /* CPU owns the cache */
    cpu_up   = std::make_unique<RiscV>(*cache_up);

    /* hand out raw pointers to the caller */
    io_out  = mmio_raw;
    cpu_out = cpu_up.get();

    /* seed a 2‑instr ROM (nop + self‑halt) */
    constexpr std::uint32_t nop  = 0x00000013;   // addi x0,x0,0
    constexpr std::uint32_t halt = 0x0000006f;   // jal  x0,0
    cache_up->store_word(0, nop);
    cache_up->store_word(4, halt);

    /* splash screen */
    // gfx::draw_title_screen(framebuffer);
    text::BitmapFont::drawText(
        framebuffer,           // fb pointer
        128u,                  // fbWidth
        128u,                  // fbHeight
        20,                     // x origin
        30,                    // y origin
        "RISC-V GAME",         // text to draw
        0xFF                   // white color (RGB332)
    );
}

} // namespace rv