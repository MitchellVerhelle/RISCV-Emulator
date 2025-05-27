#pragma once
#include "riscv_types.hpp"
#include <array>
#include <stdexcept>
#include <utility>
#include <format>

namespace rv::detail {

/*
Primary template - NO implementation on purpose.
Attempting to use Decoder<Opcode::XYZ>::decode() without a matching specialisation triggers a compile-time error.
*/
template <Opcode O>
struct Decoder
{
    [[nodiscard]] static Instr decode(std::uint32_t)
    {
        throw std::runtime_error(std::format("Illegal primary opcode 0x{:02x}", static_cast<unsigned>(O)));
    }
};

/*
Specialisations - one per primary opcode we understand
*/
template <>
struct Decoder<Opcode::OP>
{
    [[nodiscard]] static RType decode(std::uint32_t w) noexcept
    {
        auto u5 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x1F); };
        auto u3 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x07); };
        auto u7 = [](std::uint32_t x)       { return static_cast<std::uint8_t>(x>>25);       };
        return RType{ u5(w,7), u5(w,15), u5(w,20), u3(w,12), u7(w) };
    }
};

template <>
struct Decoder<Opcode::OP_IMM>
{
    [[nodiscard]] static IType decode(std::uint32_t w) noexcept
    {
        auto u5 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x1F); };
        auto u3 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x07); };
        return IType{ u5(w,7), u5(w,15), u3(w,12), sign_extend(w>>20,12) };
    }
};

template <>
struct Decoder<Opcode::LOAD> : Decoder<Opcode::OP_IMM> {}; // same layout

template <>
struct Decoder<Opcode::JALR> : Decoder<Opcode::OP_IMM> {}; // same layout

template <>
struct Decoder<Opcode::STORE>
{
    [[nodiscard]] static SType decode(std::uint32_t w) noexcept
    {
        auto u5 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x1F); };
        auto u3 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x07); };
        return SType{ u5(w,15), u5(w,20), u3(w,12), sign_extend( (u5(w,7)) | ((w>>25)<<5), 12) };
    }
};

template <>
struct Decoder<Opcode::BRANCH>
{
    [[nodiscard]] static BType decode(std::uint32_t w) noexcept
    {
        auto u5 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x1F); };
        auto u3 = [](std::uint32_t x, int s){ return static_cast<std::uint8_t>((x>>s)&0x07); };
        std::int32_t imm = sign_extend( ((w>>8)&0x0F)<<1   |  // imm[4:1]
                                        ((w>>25)&0x3F)<<5  |  // imm[10:5]
                                        ((w>>7)&0x01)<<11  |  // imm[11]
                                        (w>>31)<<12, 13);     // imm[12]
        return BType{ u5(w,15), u5(w,20), u3(w,12), imm };
    }
};

template <>
struct Decoder<Opcode::JAL>
{
    [[nodiscard]] static UJType decode(std::uint32_t w) noexcept
    {
        std::int32_t imm = sign_extend(((w >> 12) & 0xFF) << 12 |  // imm[19:12]
                                       ((w >> 20) & 0x1) << 11 |   // imm[11]
                                       ((w >> 21) & 0x3FF) << 1 |  // imm[10:1]
                                       (w >> 31) << 20, 21);       // imm[20]
        return UJType{ static_cast<std::uint8_t>((w >> 7) & 0x1F), imm }; // rd and imm
    }
};

template <>
struct Decoder<Opcode::LUI>
{
    [[nodiscard]] static UType decode(std::uint32_t w) noexcept
    {
        // rd = bits[11:7], imm = upper 20 bits << 12 (signed)
        auto rd  = static_cast<std::uint8_t>((w >> 7) & 0x1F);
        // high 20 bits are bits[31:12], low 12 are zero
        std::int32_t imm = static_cast<std::int32_t>(w & 0xFFFFF000);
        return UType{ rd, imm };
    }
};

template <>
struct Decoder<Opcode::AUIPC>
{
    [[nodiscard]] static UType decode(std::uint32_t w) noexcept
    {
        auto rd  = static_cast<std::uint8_t>((w >> 7) & 0x1F);
        std::int32_t imm = static_cast<std::int32_t>(w & 0xFFFFF000);
        return UType{ rd, imm };
    }
};

/*
Helper: one unique wrapper per opcode index
*/
template <std::size_t I>
constexpr Instr decode_wrap(std::uint32_t w)
{
    return Decoder<static_cast<Opcode>(I)>::decode(w);
}

/*
dispatch table - Inspired by Matt Godbolt
*/
template <std::size_t... Is>
constexpr auto build_table(std::index_sequence<Is...>)
{
    using Fn = Instr(*)(std::uint32_t);
    return std::array<Fn, 128>{ &decode_wrap<Is>... };
}

inline constexpr auto decoder_table = build_table(std::make_index_sequence<128>{});

} // namespace rv::detail
