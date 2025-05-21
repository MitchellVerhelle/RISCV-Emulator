#pragma once
#include "memory_bus.hpp"
#include <array>
#include <format>
#include <stdexcept>
#include <variant>
#include <optional>

namespace rv {

/*----------------  helper: sign‑extend  ----------------------------*/
constexpr std::int32_t sign_extend(std::uint32_t v, unsigned bits) noexcept
{
    const std::uint32_t m = 1u << (bits-1);
    return static_cast<std::int32_t>((v ^ m) - m);
}

/*----------------  Decoder structs (primary opcode → Data variant) --*/
enum class Opcode : std::uint8_t {
    OP   = 0b0110011,
    OP_IMM = 0b0010011,
    LOAD = 0b0000011,
    STORE= 0b0100011,
    BRANCH=0b1100011,
    LUI  = 0b0110111,
    AUIPC= 0b0010111,
    JAL  = 0b1101111,
    JALR = 0b1100111,
};

/* Generic R‑type data layout */
struct RType { std::uint8_t rd, rs1, rs2, funct3; std::uint8_t funct7; };
struct IType { std::uint8_t rd, rs1, funct3; std::int32_t imm; };
struct SType { std::uint8_t rs1, rs2, funct3; std::int32_t imm; };
struct BType { std::uint8_t rs1, rs2, funct3; std::int32_t imm; };

using Instr = std::variant<RType,IType,SType,BType>;

[[nodiscard]] inline Instr decode(std::uint32_t w)
{
    std::uint8_t opc = w & 0x7F;

    auto u5  = [](std::uint32_t w, int s){ return static_cast<std::uint8_t>((w >> s) & 0x1F); };
    auto u3  = [](std::uint32_t w, int s){ return static_cast<std::uint8_t>((w >> s) & 0x07); };
    auto u7  = [](std::uint32_t w)       { return static_cast<std::uint8_t>(w >> 25);        };
    switch (static_cast<Opcode>(opc)) {
        case Opcode::OP:        // R‑type (ADD, SUB, ...)
            RType{ u5(w,7), u5(w,15), u5(w,20), u3(w,12), u7(w) };
        case Opcode::OP_IMM:    // ADDI, SLTI, ...
        case Opcode::LOAD:      // LB/LH/LW
        case Opcode::JALR:      // jalr
            return IType{ u5(w,7), u5(w,15), u3(w,12),
                          sign_extend(w >> 20, 12) };
        case Opcode::STORE:     // SB/SH/SW
            return SType{ u5(w,15), u5(w,20), u3(w,12),
                          sign_extend( (u5(w,7)) | ((w >> 25) << 5), 12) };
        case Opcode::BRANCH:    // BEQ/BNE/...
            return BType{ u5(w,15), u5(w,20), u3(w,12),
                          sign_extend( ((w >>  8) & 0x0F)  << 1  |   // imm[4:1]
                                        ((w >> 25) & 0x3F) << 5  |   // imm[10:5]
                                        ((w >>  7) & 0x01) << 11 |   // imm[11]
                                        ( w >> 31 )        << 12, 13) }; // imm[12]
        default: throw std::runtime_error(std::format("Unknown opcode 0x{:02x}", opc));
    }
}

/*----------------  CPU core  --------------------------------------*/
class RiscV
{
  public:
    explicit RiscV(MemoryBus& m) : mem_{m} {}

    void step();
    [[nodiscard]] std::uint32_t pc()     const noexcept { return pc_; }
    [[nodiscard]] std::uint32_t reg(int i) const noexcept { return regs_[i]; }
    [[nodiscard]] MemoryBus&    mem()         noexcept { return mem_; }

  private:
    std::array<std::uint32_t,32> regs_{};
    std::uint32_t pc_{0};
    MemoryBus&    mem_;

    /* helpers */
    void write_reg(std::uint8_t rd, std::uint32_t v) noexcept
    { if (rd) regs_[rd]=v; }
};

} // namespace rv
