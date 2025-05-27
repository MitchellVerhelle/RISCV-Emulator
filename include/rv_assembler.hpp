// include/rv_assembler.hpp
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define CTRE_ENABLE_LITERALS
#include <ctre.hpp>           // CTRE v3.10.4 single header   
#include "riscv_types.hpp"

namespace rv {

/* ------------------------------------------------------------------ */
/* 1. helpers                                                         */
/* ------------------------------------------------------------------ */
constexpr std::array reg_names{
    "x0","x1","x2","x3","x4","x5","x6","x7",
    "x8","x9","x10","x11","x12","x13","x14","x15",
    "x16","x17","x18","x19","x20","x21","x22","x23",
    "x24","x25","x26","x27","x28","x29","x30","x31"
};

constexpr std::uint8_t regnum(std::string_view s)
{
    auto it = std::find(reg_names.begin(), reg_names.end(), s);
    if (it == reg_names.end())
        throw std::invalid_argument(std::format("bad reg '{}'", s));
    return static_cast<std::uint8_t>(it - reg_names.begin());
}

/* RISC-V bit-pack helpers (R/I/S/B) --------------------------------- */
struct EncR { std::uint8_t rd, rs1, rs2, f3, f7; };
struct EncI { std::uint8_t rd, rs1, f3; std::int32_t imm; };
struct EncS { std::uint8_t rs2, rs1, f3; std::int32_t imm; };
struct EncB { std::uint8_t rs2, rs1, f3; std::int32_t imm; };

constexpr std::uint32_t R(const EncR& e, Opcode opc) noexcept
{
    return (static_cast<std::uint32_t>(e.f7 << 25)) | (static_cast<std::uint32_t>(e.rs2 << 20)) | (static_cast<std::uint32_t>(e.rs1 << 15)) |
           (static_cast<std::uint32_t>(e.f3 << 12)) | (static_cast<std::uint32_t>(e.rd << 7))   | static_cast<std::uint32_t>(opc);
}
constexpr std::uint32_t I(const EncI& e, Opcode opc) noexcept
{
    return (static_cast<std::uint32_t>((e.imm & 0xFFF) << 20)) | (static_cast<std::uint32_t>(e.rs1 << 15)) |
           (static_cast<std::uint32_t>(e.f3 << 12)) | (static_cast<std::uint32_t>(e.rd << 7)) | static_cast<std::uint32_t>(opc);
}
constexpr std::uint32_t S(const EncS& e, Opcode opc) noexcept
{
    const std::uint32_t imm = static_cast<std::uint32_t>(e.imm) & 0xFFF;
    return (static_cast<std::uint32_t>((imm & 0xFE0) << 20)) | (static_cast<std::uint32_t>(e.rs2 << 20)) | (static_cast<std::uint32_t>(e.rs1 << 15)) |
           (static_cast<std::uint32_t>(e.f3 << 12)) | ((imm & 0x1F) << 7) | static_cast<std::uint32_t>(opc);
}
constexpr std::uint32_t B(const EncB& e, Opcode opc) noexcept
{
    const std::uint32_t imm = static_cast<std::uint32_t>(e.imm) & 0x1FFF;
    return ((imm & 0x1000) << 19) | ((imm & 0x7E0) << 20) |
           ((imm & 0x1E) << 7)    | ((imm & 0x800) >> 4)  |
           (static_cast<std::uint32_t>(e.rs2 << 20)) | (static_cast<std::uint32_t>(e.rs1 << 15)) |
           (static_cast<std::uint32_t>(e.f3 << 12)) | static_cast<std::uint32_t>(opc);
}

/* ------------------------------------------------------------------ */
/* 2.  assemble a single line                                         */
/* ------------------------------------------------------------------ */
inline std::optional<std::uint32_t>
assemble_line(std::string_view ln,
              std::size_t       pc,
              const std::unordered_map<std::string,std::size_t>& labels)
{
    /* ---- R-type: add / sub ------------------------------------- */
    if (auto m = ctre::match<"add\\s+(\\w+),\\s*(\\w+),\\s*(\\w+)">(ln))
        return R({ regnum(m.get<1>()), regnum(m.get<2>()), regnum(m.get<3>()),
                   0b000, 0b0000000 }, Opcode::OP);

    if (auto m = ctre::match<"sub\\s+(\\w+),\\s*(\\w+),\\s*(\\w+)">(ln))
        return R({ regnum(m.get<1>()), regnum(m.get<2>()), regnum(m.get<3>()),
                   0b000, 0b0100000 }, Opcode::OP);

    /* ---- I-type ------------------------------------------------- */
    if (auto m = ctre::match<"addi\\s+(\\w+),\\s*(\\w+),\\s*(-?\\d+)">(ln))
        return I({ regnum(m.get<1>()), regnum(m.get<2>()), 0b000,
                   std::stoi(std::string{m.get<3>()}) }, Opcode::OP_IMM);
    
    // jalr rd, rs1, imm
    if (auto m = ctre::match<"jalr\\s+(\\w+),\\s*(\\w+),\\s*(-?\\d+)">(ln))
        return I({ regnum(m.get<1>()), regnum(m.get<2>()), 0b000,
                   std::stoi(std::string{m.get<3>()}) },
                Opcode::JALR);

    if (auto m = ctre::match<"lw\\s+(\\w+),\\s*(-?\\d+)\\(\\s*(\\w+)\\s*\\)">(ln))
        return I({ regnum(m.get<1>()), regnum(m.get<3>()), 0b010,
                   std::stoi(std::string{m.get<2>()}) }, Opcode::LOAD);

    if (auto m = ctre::match<"jalr\\s+(\\w+),\\s*(-?\\d+)\\(\\s*(\\w+)\\s*\\)">(ln))
        return I({ regnum(m.get<1>()), regnum(m.get<3>()), 0b000,
                   std::stoi(std::string{m.get<2>()}) }, Opcode::JALR);

    /* ---- S-type ------------------------------------------------- */
    if (auto m = ctre::match<"sw\\s+(\\w+),\\s*(-?\\d+)\\(\\s*(\\w+)\\s*\\)">(ln))
        return S({ regnum(m.get<1>()), regnum(m.get<3>()), 0b010,
                   std::stoi(std::string{m.get<2>()}) }, Opcode::STORE);

    /* ---- B-type ------------------------------------------------- */
    if (auto m = ctre::match<"bne\\s+(\\w+),\\s*(\\w+),\\s*(\\w+)">(ln)) {
        auto tgt = labels.at(std::string{m.get<3>()});
        std::int32_t off = static_cast<std::int32_t>(tgt) -
                           static_cast<std::int32_t>(pc);
        return B({ regnum(m.get<2>()), regnum(m.get<1>()), 0b001, off },
                 Opcode::BRANCH);
    }

    if (auto m = ctre::match<"beq\\s+(\\w+),\\s*(\\w+),\\s*(\\w+)">(ln)) {
        auto tgt = labels.at(std::string{m.get<3>()});
        std::int32_t off = static_cast<std::int32_t>(tgt) -
                           static_cast<std::int32_t>(pc);
        return B({ regnum(m.get<2>()), regnum(m.get<1>()), 0b000, off },
                 Opcode::BRANCH);
    }

    /* ---- no pattern matched ------------------------------------ */
    return std::nullopt;
}

/* ------------------------------------------------------------------ */
/* 3.  public driver                                                  */
/* ------------------------------------------------------------------ */
[[nodiscard]]
inline std::vector<std::uint32_t>
assemble(std::string_view src)
{
    /* split lines & strip comments -------------------------------------- */
    std::vector<std::string_view> lines;
    for (std::size_t b = 0, e; b < src.size(); b = e + 1) {
        e = src.find_first_of("\r\n", b);
        if (e == std::string_view::npos) e = src.size();
        auto ln = src.substr(b, e - b);

        if (auto c = ln.find('#'); c != std::string_view::npos) ln = ln.substr(0, c);

        auto first = ln.find_first_not_of(" \t");
        if (first == std::string_view::npos) { lines.emplace_back(); continue; }
        auto last = ln.find_last_not_of(" \t");
        lines.push_back(ln.substr(first, last - first + 1));
    }

    /* pass 1: label table ------------------------------------------------ */
    std::unordered_map<std::string,std::size_t> labels;
    std::size_t pc = 0;
    for (auto& ln : lines) {
        if (ln.empty()) continue;

        if (auto m = ctre::match<R"(^(\w+):)">(ln)) {
            labels.emplace(std::string{m.get<1>()}, pc);
            ln.remove_prefix(m.get<0>().to_view().size());

            // set ln to empty if label only
            if (auto pos = ln.find_first_not_of(" \t");
                pos != std::string_view::npos)
                ln.remove_prefix(pos);
            else
                ln = {};
        }
        if (!ln.empty()) pc += 4;
    }

    /* pass 2: encode ----------------------------------------------------- */
    std::vector<std::uint32_t> words;
    words.reserve(pc / 4);

    pc = 0;
    for (auto const& ln : lines) {
        if (ln.empty()) continue;

        auto word = assemble_line(ln, pc, labels);
        if (!word)
            throw std::runtime_error(std::format("syntax error: '{}'", ln));

        words.push_back(*word);
        pc += 4;
    }
    return words;
}

} // namespace rv
