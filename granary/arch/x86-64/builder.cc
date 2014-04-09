/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/arch/base.h"

#include "granary/arch/x86-64/builder.h"
#include "granary/arch/x86-64/instruction.h"

namespace granary {
namespace arch {

namespace {
// Returns the bit width of an immediate integer. This is to calculate operand
// width when using the instruction builder IR.
static int ImmediateWidthBits(uint64_t imm) {
  enum : uint64_t {
    WIDTH_8   = 0x0FFUL,
    WIDTH_16  = WIDTH_8 | (WIDTH_8 << 8),
    WIDTH_32  = WIDTH_16 | (WIDTH_16 << 16)
  };
  if (!imm) return 1;
  if ((imm | ~WIDTH_8) == imm) return 8;  // Signed.
  if ((imm & WIDTH_8) == imm) return 8;  // Unsigned.

  if ((imm | ~WIDTH_16) == imm) return 16;  // Signed.
  if ((imm & WIDTH_16) == imm) return 16;  // Unsigned.

  if ((imm | ~WIDTH_32) == imm) return 32;  // Signed.
  if ((imm & WIDTH_32) == imm) return 32;  // Unsigned.
  return 64;
}
}  // namespace

// Initialize an emptry Granary `arch::Instruction` from a XED iclass,
// category, and the number of explicit operands.
void BuildInstruction(Instruction *instr, xed_iclass_enum_t iclass,
                      xed_category_enum_t category) {
  memset(instr, 0, sizeof *instr);
  instr->iclass = iclass;
  instr->category = category;

  // These are only really atomic if there's a memory op.
  //
  // TODO(pag): There might be other categories of atomic operations (with
  //            XED_ATTRIBUTE_HLE_ACQ_ABLE andXED_ATTRIBUTE_HLE_REL_ABLE, but
  //            only if they have memory operations. This might come up where
  //            an instruction is initially not atomic (e.g. reg->reg), but then
  //            is modified by a tool to be mem->reg or reg->mem, thus making it
  //            atomic.
  instr->is_atomic = XED_ICLASS_XCHG == iclass ||
                     XED_CATEGORY_SEMAPHORE == category;
  if (GRANARY_UNLIKELY(XED_ICLASS_LEA == iclass)) {
    instr->ops[1].is_effective_address = true;
  }
}

// Add this register as an operand to the instruction `instr`.
void RegisterBuilder::Build(Instruction *instr) {
  auto &op(instr->ops[instr->num_explicit_ops++]);
  op.type = XED_ENCODER_OPERAND_TYPE_REG;
  op.reg = reg;
  op.rw = action;
  op.width = static_cast<int8_t>(reg.BitWidth());

  // Registers AH through BH are tricky to handle due to their location, so we
  // treat them as hard requirements for virtual register scheduling.
  if (reg.IsNative()) {
    auto arch_reg = static_cast<xed_reg_enum_t>(reg.EncodeToNative());
    if (XED_REG_AH <= arch_reg && arch_reg <= XED_REG_BH) {
      op.is_sticky = true;
    }
  }
}

// Add this immediate as an operand to the instruction `instr`.
void ImmediateBuilder::Build(Instruction *instr) {
  auto &op(instr->ops[instr->num_explicit_ops++]);
  op.imm.as_uint = as_uint;
  op.type = type;
  op.rw = XED_OPERAND_ACTION_R;
  op.width = static_cast<int8_t>(ImmediateWidthBits(as_uint));
}

// Add this memory as an operand to the instruction `instr`.
void MemoryBuilder::Build(Instruction *instr) {
  auto &instr_op(instr->ops[instr->num_explicit_ops++]);
  instr_op.width = -1;  // Unknown.
  instr_op.is_compound = false;
  instr_op.is_effective_address = false;
  switch (kind) {
    case BUILD_POINTER:
      instr_op.type = XED_ENCODER_OPERAND_TYPE_PTR;
      instr_op.addr.as_ptr = ptr;
      break;
    case BUILD_REGISTER:
      instr_op.type = XED_ENCODER_OPERAND_TYPE_MEM;
      instr_op.reg = reg;
      break;
    case BUILD_OPERAND:
      auto is_effective_address = instr_op.is_effective_address;
      instr_op = op;
      instr_op.is_effective_address = is_effective_address;
      break;
  }
  instr_op.rw = action;
}

// Add this branch target as an operand to the instruction `instr`.
void BranchTargetBuilder::Build(Instruction *instr) {
  auto &op(instr->ops[instr->num_explicit_ops++]);
  op.branch_target.as_pc = pc;
  op.rw = XED_OPERAND_ACTION_R;
  op.type = XED_ENCODER_OPERAND_TYPE_BRDISP;
  op.width = arch::ADDRESS_WIDTH_BITS;
}

}  // namespace arch
}  // namespace granary