// src/riscv.cpp
#include "riscv.hpp"
#include "riscv_types.hpp"
#include <format>
#include <stdexcept>

namespace rv {

void RiscV::step()
{
    auto word_opt = mem_.load_word(pc_);
    if (!word_opt) throw std::runtime_error("Fetch fault");
    uint32_t raw = *word_opt;

    auto inst = decode(raw);

    std::visit([&](auto&& d){
        using T = std::decay_t<decltype(d)>;

        if constexpr (std::is_same_v<T, RType>) {
            // … existing R-type (add/sub) …
            switch ((d.funct7 << 3) | d.funct3) {
              case 0b000'0000000: write_reg(d.rd, regs_[d.rs1] + regs_[d.rs2]); break; // ADD
              case 0b000'0100000: write_reg(d.rd, regs_[d.rs1] - regs_[d.rs2]); break; // SUB
              default: throw std::runtime_error("Unimpl R-type");
            }
            pc_ += 4;
        }
        else if constexpr (std::is_same_v<T, IType>) {
            // … existing I-type (ADDI, LOAD, JALR) …
            switch (static_cast<Opcode>(raw & 0x7F)) {
              case Opcode::OP_IMM:
                if (d.funct3 == 0) {
                  write_reg(d.rd, regs_[d.rs1] + static_cast<uint32_t>(d.imm)); // ADDI
                } else throw std::runtime_error("Unimpl OP-IMM");
                pc_ += 4;
                break;

              case Opcode::LOAD: {
                auto addr = regs_[d.rs1] + static_cast<uint32_t>(d.imm);
                write_reg(d.rd, mem_.load_word(addr).value_or(0));
                pc_ += 4;
                break;
              }

              case Opcode::JALR: {
                uint32_t link   = pc_ + 4;
                uint32_t target = regs_[d.rs1] + static_cast<uint32_t>(d.imm);
                pc_ = target & ~uint32_t{1};
                write_reg(d.rd, link);
                break;
              }

              default:
                throw std::runtime_error("Unimpl I-type");
            }
        }
        else if constexpr (std::is_same_v<T, SType>) {
            // … existing S-type (stores) …
            uint32_t addr = regs_[d.rs1] + static_cast<uint32_t>(d.imm);
            switch (d.funct3) {
              case 0: mem_.store_word(addr, regs_[d.rs2] & 0xFF);      break; // SB
              case 1: mem_.store_word(addr, regs_[d.rs2] & 0xFFFF);    break; // SH
              case 2: mem_.store_word(addr, regs_[d.rs2]);             break; // SW
              default: throw std::runtime_error("Unimpl STORE");
            }
            pc_ += 4;
        }
        else if constexpr (std::is_same_v<T, BType>) {
            // … existing B-type (branches) …
            bool take = false;
            switch (d.funct3) {
              case 0: take = regs_[d.rs1] == regs_[d.rs2]; break; // BEQ
              case 1: take = regs_[d.rs1] != regs_[d.rs2]; break; // BNE
              case 4: take = int32_t(regs_[d.rs1]) <  int32_t(regs_[d.rs2]); break; // BLT
              case 5: take = int32_t(regs_[d.rs1]) >= int32_t(regs_[d.rs2]); break; // BGE
              case 6: take = regs_[d.rs1] <  regs_[d.rs2]; break; // BLTU
              case 7: take = regs_[d.rs1] >= regs_[d.rs2]; break; // BGEU
              default: throw std::runtime_error("Unimpl BRANCH");
            }
            pc_ += uint32_t(take ? d.imm : 4);
        }
        else if constexpr (std::is_same_v<T, UType>) {
            // LUI / AUIPC
            //   UType.imm already holds the 20-bit <<12 immediate
            write_reg(d.rd, static_cast<uint32_t>(d.imm));
            pc_ += 4;
        }
        else if constexpr (std::is_same_v<T, UJType>) {
            // JAL
            uint32_t link = pc_ + 4;
            pc_ = uint32_t(int32_t(pc_) + d.imm);
            write_reg(d.rd, link);
        }
        else {
            // any other Instr variant: trap
            throw std::runtime_error("Unrecognized instruction variant in visit");
        }

    }, inst);
}

} // namespace rv
