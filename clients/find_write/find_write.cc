/* Copyright 2014 Peter Goodman, all rights reserved. */

#include <granary.h>

using namespace granary;

GRANARY_DEFINE_mask(address_mask, std::numeric_limits<uintptr_t>::max(),
    "Mask that is used to filter addresses. If all bits are set then all "
    "addresses are accepted.\n"
    "\n"
    "If `(addr & addr_mask) != 0` then the write is recorded "
    "into an in-memory log. Log entries contain four "
    "components:\n"
    "  1) Target address of the write.\n"
    "  2) Value being written to memory.\n"
    "  3) Application address of the instruction doing the\n"
    "     write.\n"
    "  4) Cache address of the instruction doing the write.",

    "find_write");

GRANARY_DEFINE_mask(value_mask, std::numeric_limits<uintptr_t>::max(),
    "Mask that is used to filter values. If all bits are set then all values "
    "are accepted.\n"
    "\n"
    "If `(value & value_mask) != 0` then the write is recorded "
    "into an in-memory log.",

    "find_write");

namespace {

// Report an 8-bit memory write.
static void ReportWrite8(const char *mod_name, uint64_t offset,
                         void *addr, uint8_t value) {
  os::Log(os::LogDebug, "1,%s,%lx,%p,%x\n", mod_name, offset, addr, value);
}

// Report a 16-bit memory write.
static void ReportWrite16(const char *mod_name, uint64_t offset,
                          void *addr, uint16_t value) {
  os::Log(os::LogDebug, "2,%s,%lx,%p,%x\n", mod_name, offset, addr, value);
}

// Report a 32-bit memory write.
static void ReportWrite32(const char *mod_name, uint64_t offset,
                          void *addr, uint32_t value) {
  os::Log(os::LogDebug, "4,%s,%lx,%p,%x\n", mod_name, offset, addr, value);
}

// Report a 64-bit memory write.
static void ReportWrite64(const char *mod_name, uint64_t offset,
                          void *addr, uint64_t value) {
  os::Log(os::LogDebug, "8,%s,%lx,%p,%lx\n",  mod_name, offset, addr, value);
}

// Choose what function to use to log the memory write.
static AppPC GetWriteReporter(Operand &op) {
  switch (op.BitWidth()) {
    case 8:
      return UnsafeCast<AppPC>(ReportWrite8);
    case 16:
      return UnsafeCast<AppPC>(ReportWrite16);
    case 32:
      return UnsafeCast<AppPC>(ReportWrite32);
    case 64:
    default:
      return UnsafeCast<AppPC>(ReportWrite64);
  }
}
}  // namespace

// Example tool that instruments memory writes of the form:
//    MOV [addr_reg], value_reg
//    MOV [addr_reg], value_imm
// This tool logs all writes where `0 != (addr_reg & FLAG_address_mask)` and
// `0 != (value_reg/_imm & FLAG_value_mask)`.
class MemoryWriteInstrumenter : public InstrumentationTool {
 public:
  virtual ~MemoryWriteInstrumenter(void) = default;

  // Writing an immediate constant to memory. Avoid a check on the value mask.
  void InstrumentMemoryWrite(DecodedBasicBlock *block, os::ModuleOffset loc,
                             NativeInstruction *instr, VirtualRegister dst_addr,
                             ImmediateOperand &value) {
    if (FLAG_value_mask && !(FLAG_value_mask & value.UInt())) return;
    RegisterOperand address(dst_addr);
    ImmediateOperand address_mask(FLAG_address_mask, arch::ADDRESS_WIDTH_BYTES);

    lir::InlineAssembly asm_({&address, &address_mask, &value});
    if (FLAG_address_mask &&
        std::numeric_limits<uintptr_t>::max() != FLAG_address_mask) {
      asm_.InlineBefore(instr,
          "MOV r64 %4, i64 %1;"
          "TEST r64 %4, r64 %0;"
          "JZ l %3;"_x86_64);
    }
    instr->InsertBefore(lir::CallWithArgs(block, GetWriteReporter(value),
                                          loc.module->Name(), loc.offset,
                                          address, value));

    asm_.InlineBefore(instr, "LABEL %3:"_x86_64);
  }

  // Writing the value of a register to memory.
  void InstrumentMemoryWrite(DecodedBasicBlock *block, os::ModuleOffset loc,
                             NativeInstruction *instr, VirtualRegister dst_addr,
                             RegisterOperand &value) {
    RegisterOperand address(dst_addr);
    ImmediateOperand address_mask(FLAG_address_mask, arch::ADDRESS_WIDTH_BYTES);
    ImmediateOperand value_mask(FLAG_value_mask, arch::ADDRESS_WIDTH_BYTES);

    lir::InlineAssembly asm_({&address, &address_mask, &value, &value_mask});

    if (FLAG_address_mask &&
        std::numeric_limits<uintptr_t>::max() != FLAG_address_mask) {
      asm_.InlineBefore(instr,
          "MOV r64 %5, i64 %1;"
          "TEST r64 %5, r64 %0;"
          "JZ l %4;"_x86_64);
    }
    if (FLAG_value_mask &&
        std::numeric_limits<uintptr_t>::max() != FLAG_value_mask) {
      asm_.InlineBefore(instr,
          "MOV r64 %5, i64 %3;"
          "TEST r64 %5, r64 %2;"
          "JZ l %4;"_x86_64);
    }
    instr->InsertBefore(lir::CallWithArgs(block, GetWriteReporter(value),
                                          loc.module->Name(), loc.offset,
                                          address, value));
    asm_.InlineBefore(instr, "LABEL %4:");
  }

  // Instrument every memory write instruction.
  virtual void InstrumentBlock(DecodedBasicBlock *block) {
    AppPC pc(block->StartAppPC());
    auto module = os::ModuleContainingPC(pc);
    for (auto instr : block->Instructions()) {
      if (auto ninstr = DynamicCast<NativeInstruction *>(instr)) {
        if(auto instr_pc = ninstr->DecodedPC()) {
          pc = instr_pc;
        }

        if (!StringsMatch("MOV", ninstr->OpCodeName())) continue;

        MemoryOperand dst;
        VirtualRegister dst_addr;
        ImmediateOperand src_imm;
        RegisterOperand src_reg;

        if (ninstr->MatchOperands(WriteTo(dst), ReadFrom(src_imm))) {
          if (dst.MatchRegister(dst_addr)) {
            InstrumentMemoryWrite(block, module->OffsetOfPC(pc), ninstr,
                                  dst_addr, src_imm);
          }
        } else if (ninstr->MatchOperands(WriteTo(dst), ReadFrom(src_reg))) {
          if (dst.MatchRegister(dst_addr)) {
            InstrumentMemoryWrite(block, module->OffsetOfPC(pc), ninstr,
                                  dst_addr, src_reg);
          }
        }
      }
    }
  }
};

// Initialize the `find_write` tool.
GRANARY_CLIENT_INIT({
  if (HAS_FLAG_address_mask && !FLAG_address_mask) return;
  if (HAS_FLAG_value_mask && !FLAG_value_mask) return;
  RegisterInstrumentationTool<MemoryWriteInstrumenter>("find_write");
})
