#include "mmio_window.hpp"
#include "concurrent_hash_table.hpp"
#include "cache.hpp"
#include "riscv.hpp"
#include "text/bitmap_font.hpp"
#include <memory>

namespace rv {

static std::unique_ptr<ConcurrentHashTable<std::uint32_t,std::uint32_t>> dram_up;
static std::unique_ptr<Cache> cache_up;
static std::unique_ptr<RiscV> cpu_up;

/* 128-by-128 RGB332 framebuffer storage */
static std::uint8_t framebuffer[128 * 128]{};

void build_system(MmioWindow*& io_out, RiscV*& cpu_out)
{
    dram_up = std::make_unique<ConcurrentHashTable<std::uint32_t,std::uint32_t>>();
    auto mmio_ptr = std::make_unique<MmioWindow>(std::move(dram_up));
    mmio_ptr->framebuffer = framebuffer;
    MmioWindow* mmio_raw = mmio_ptr.get();

    cache_up = std::make_unique<Cache>(64, 2, std::move(mmio_ptr));
    cpu_up   = std::make_unique<RiscV>(*cache_up);

    io_out  = mmio_raw;
    cpu_out = cpu_up.get();

    // seed ROM: NOP + self-halt
    constexpr std::uint32_t nop  = 0x00000013;
    constexpr std::uint32_t halt = 0x0000006F;
    cache_up->store_word(0, nop);
    cache_up->store_word(4, halt);

    // vertical blue-green gradient 
    for (int y = 0; y < 128; ++y) {
        std::uint8_t shade = static_cast<std::uint8_t>(32 + y);  // 0x20‥0xA0
        for (int x = 0; x < 128; ++x)
            framebuffer[y * 128 + x] = shade;
    }

    // centre “RISC-V GAME” in white
    constexpr const char* title = "RISC-V GAME";
    constexpr int txt_len_px = text::BitmapFont::charWidth * 11; // 88 px
    const int x0 = (128 - txt_len_px) / 2;   // 20
    const int y0 = (128 - text::BitmapFont::charHeight) / 2; // 60
    text::BitmapFont::drawText(framebuffer, 128, 128, x0, y0, title, 0xFF);
}

} // namespace rv