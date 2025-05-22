#pragma once
#include "memory_bus.hpp"
#include <cstdint>
#include <optional>
#include <memory>

namespace rv {

/* host‑side memory‑mapped I/O window */
class MmioWindow : public MemoryBus
{
  public:
    explicit MmioWindow(std::unique_ptr<MemoryBus> next)   // ← ctor
        : next_{std::move(next)} {}

    std::uint8_t* framebuffer = nullptr;   // 128×128 byte‑indexed FB
    std::uint8_t  gpio_in     = 0;         // buttons
    std::uint8_t  audio_note  = 0;         // tone id

    std::optional<std::uint32_t> load_word(std::uint32_t) override;
    bool                         store_word(std::uint32_t,
                                            std::uint32_t) override;
  private:
    std::unique_ptr<MemoryBus> next_;      // DRAM or next cache
};

} // namespace rv
