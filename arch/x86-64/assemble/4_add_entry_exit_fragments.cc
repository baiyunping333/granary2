/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL
#define GRANARY_ARCH_INTERNAL

#include "arch/x86-64/instruction.h"

#include "granary/code/fragment.h"

namespace granary {
namespace arch {

// Table mapping each iclass to the set of read and written flags by *any*
// selection of that iclass.
extern const FlagsSet IFORM_FLAGS[];

// Table telling us how flags are used by a particular iclass.
extern const FlagActions ICLASS_FLAG_ACTIONS[];

enum : unsigned {
  ALL_AFLAGS_WITH_DF = 3285U,
  ALL_AFLAGS_WITHOUT_DF = 2261U,
  ZF = (1U << 6U)
};

// Visits an instructions within the fragment and revives/kills architecture-
// specific flags stored in the `FlagUsageInfo` object.
void VisitInstructionFlags(const arch::Instruction &instr,
                           FlagUsageInfo *flags) {
  const auto &instr_flags(arch::IFORM_FLAGS[instr.iform]);
  auto read_flags = instr_flags.read.flat;
  auto written_flags = instr_flags.written.flat;

  if (instr.has_prefix_rep || instr.has_prefix_repne) {
    read_flags |= ZF;
    written_flags |= ZF;
  }

  flags->all_written_flags |= written_flags & ALL_AFLAGS_WITH_DF;
  flags->all_read_flags |= read_flags & ALL_AFLAGS_WITH_DF;

  if (ICLASS_FLAG_ACTIONS[instr.iclass].is_conditional_write) {
    flags->entry_live_flags |= written_flags & ALL_AFLAGS_WITH_DF;
  } else {
    flags->entry_live_flags &= ~written_flags & ALL_AFLAGS_WITH_DF;
  }

  flags->entry_live_flags |= read_flags & ALL_AFLAGS_WITH_DF;
}

// Returns a bitmap representing all arithmetic flags being live.
uint32_t AllArithmeticFlags(void) {
  return ALL_AFLAGS_WITHOUT_DF;
  /*
  // For documentation purposes only.
  xed_flag_set_t flags;
  flags.s.of = 1;
  flags.s.sf = 1;
  flags.s.zf = 1;
  flags.s.af = 1;
  flags.s.pf = 1;
  flags.s.cf = 1;
  return flags.flat;
  */
}

}  // namespace arch
}  // namespace granary
