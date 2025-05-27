#pragma once
#include <cstdint>
#include <optional>

namespace rv {

/*
Generic byte-addressable memory bus (word-aligned accesses only).
*/
struct MemoryBus
{
    virtual std::optional<std::uint32_t> load_word(std::uint32_t addr) = 0;
    virtual bool store_word(std::uint32_t addr, std::uint32_t value) = 0;
    virtual ~MemoryBus() = default;
};

} // namespace rv
