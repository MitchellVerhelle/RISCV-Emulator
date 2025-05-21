#include "riscv.hpp"
#include <format>

namespace rv {

void RiscV::step()
{
    auto word_opt = mem_.load_word(pc_);
    if (!word_opt) throw std::runtime_error("Fetch fault");
    std::uint32_t raw = *word_opt;

    auto inst = decode(raw);

    std::visit([&](auto&& d){
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T,RType>)
        {
            switch ((d.funct7<<3)|d.funct3) {
                case 0b000'0000000: write_reg(d.rd, regs_[d.rs1] + regs_[d.rs2]); break; // add
                case 0b000'0100000: write_reg(d.rd, regs_[d.rs1] - regs_[d.rs2]); break; // sub
                default: throw std::runtime_error("Unimpl R‑type");
            }
            pc_ += 4;
        }
        else if constexpr (std::is_same_v<T,IType>)
        {
            switch (static_cast<Opcode>(raw & 0x7F)) {
                case Opcode::OP_IMM:
                    switch (d.funct3) {
                        case 0: write_reg(d.rd, regs_[d.rs1] + d.imm); break; // addi
                        default: throw std::runtime_error("Unimpl OP‑IMM");
                    }
                    pc_ += 4; break;

                case Opcode::LOAD: {
                    auto v = mem_.load_word(regs_[d.rs1]+d.imm).value_or(0);
                    write_reg(d.rd, v); pc_ += 4; break; }

                case Opcode::JALR: {
                    std::uint32_t t = pc_ + 4;
                    pc_ = (regs_[d.rs1]+d.imm) & ~1u;
                    write_reg(d.rd, t); break; }

                default: throw std::runtime_error("Unimpl I‑type");
            }
        }
        else if constexpr (std::is_same_v<T,SType>)
        {
            switch (d.funct3) {
                case 0: mem_.store_word(regs_[d.rs1]+d.imm, regs_[d.rs2]); pc_ +=4; break; // sw
                default: throw std::runtime_error("Unimpl STORE");
            }
        }
        else if constexpr (std::is_same_v<T,BType>)
        {
            bool take{};
            switch (d.funct3) {
                case 0: take = regs_[d.rs1]==regs_[d.rs2]; break; // beq
                default: throw std::runtime_error("Unimpl BRANCH");
            }
            pc_ += take? d.imm : 4;
        }
    }, inst);
}

} // namespace rv
