#pragma once
#include <cstdint>
#include <atomic>
#include <array>

namespace rv {

struct CacheLine
{
    std::uint32_t tag  = 0;
    bool          valid{false};
    bool          dirty{false};

    std::array<std::uint32_t, 4> words{};   // 16‑byte line (4×32‑bit)

    void reset() noexcept { valid = dirty = false; }
};

} // namespace rv
