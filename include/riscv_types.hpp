#pragma once
#include <cstdint>
#include <variant>

namespace rv {

constexpr std::int32_t sign_extend(std::uint32_t v, unsigned bits) noexcept
{
    const std::uint32_t m = 1u << (bits - 1);
    return static_cast<std::int32_t>((v ^ m) - m);
}

enum class Opcode : std::uint8_t {
    OP     = 0b0110011,
    OP_IMM = 0b0010011,
    LOAD   = 0b0000011,
    STORE  = 0b0100011,
    BRANCH = 0b1100011,
    LUI    = 0b0110111,
    AUIPC  = 0b0010111,
    JAL    = 0b1101111,
    JALR   = 0b1100111,
};

struct RType { std::uint8_t rd, rs1, rs2, funct3, funct7; };
struct IType { std::uint8_t rd, rs1, funct3; std::int32_t imm; };
struct SType { std::uint8_t rs1, rs2, funct3; std::int32_t imm; };
struct BType { std::uint8_t rs1, rs2, funct3; std::int32_t imm; };
struct UType { std::uint8_t rd; std::int32_t imm; };
struct UJType { std::uint8_t rd; std::int32_t imm; };

using Instr = std::variant<RType,IType,SType,BType,UType,UJType>;

} // namespace rv
