#pragma once
#include "memory_bus.hpp"
#include "riscv_decode_templates.hpp"
#include <array>
#include <format>
#include <stdexcept>
#include <variant>
#include <optional>

namespace rv {

/*
Instruction decoder
*/
[[nodiscard]] inline Instr decode(std::uint32_t word)
{
    const std::uint8_t opc = word & 0x7F;
    return detail::decoder_table[opc](word);
}


/*
CPU core
*/
class RiscV
{
  public:
    explicit RiscV(MemoryBus& m) : mem_{m} {}

    void step();
    [[nodiscard]] std::uint32_t pc() const noexcept { return pc_; }
    [[nodiscard]] std::uint32_t reg(std::size_t i) const noexcept { return regs_[i]; }
    [[nodiscard]] MemoryBus& mem() noexcept { return mem_; }

  private:
    std::array<std::uint32_t,32> regs_{};
    std::uint32_t pc_{0};
    MemoryBus& mem_;

    void write_reg(std::uint8_t rd, std::uint32_t v) noexcept
    { if (rd) regs_[rd]=v; }
};

} // namespace rv
