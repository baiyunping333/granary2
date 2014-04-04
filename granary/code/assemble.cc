/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/base/option.h"

#include "granary/code/assemble.h"

// Stages of assembly.
#include "granary/code/assemble/0_compile_inline_assembly.h"
#include "granary/code/assemble/1_relativize.h"
#include "granary/code/assemble/2_build_fragment_list.h"
#include "granary/code/assemble/3_find_live_arch_registers.h"
#include "granary/code/assemble/4_partition_fragments.h"
#include "granary/code/assemble/5_add_entry_exit_fragments.h"
#include "granary/code/assemble/6_save_and_restore_flags.h"
#include "granary/code/assemble/8_schedule_registers.h"
#include "granary/code/assemble/9_log_fragments.h"

#include "granary/logging.h"
#include "granary/util.h"

GRANARY_DEFINE_bool(debug_log_assembled_fragments, false,
    "Log the assembled fragments before doing final linking. The default is "
    "false.")

namespace granary {

// Assemble the local control-flow graph.
void Assemble(ContextInterface* env, CodeCacheInterface *code_cache,
              LocalControlFlowGraph *cfg) {

  // Compile all inline assembly instructions by parsing the inline assembly
  // instructions and doing code generation for them.
  CompileInlineAssembly(cfg);

  // "Fix" instructions that might use PC-relative operands that are now too
  // far away from their original data/targets (e.g. if the code cache is really
  // far away from the original native code in memory).
  RelativizeLCFG(code_cache, cfg);

  // Split the LCFG into fragments. The relativization step might introduce its
  // own control flow, as well as instrumentation tools. This means that
  // `DecodedBasicBlock`s no longer represent "true" basic blocks because they
  // can contain internal control-flow. This makes further analysis more
  // complicated, so to simplify things we re-split up the blocks into fragments
  // that represent the "true" basic blocks.
  auto frags = BuildFragmentList(cfg);

  // Find the live registers on entry to the fragments.
  FindLiveEntryRegsToFrags(frags);

  // Try to figure out the stack frame size on entry to / exit from every
  // fragment.
  PartitionFragmentsByStackUse(frags);

  // Add a bunch of entry/exit fragments at places where flags needs to be
  // saved/restored, and at places where GPRs need to be spilled / filled.
  AddEntryAndExitFragments(&frags);

  // Add flags saving and restoring code around injected instrumentation
  // instructions.
  SaveAndRestoreFlags(cfg, frags);

  // Schedule the virtual registers.
  ScheduleVirtualRegisters(frags);

  if (FLAG_debug_log_assembled_fragments) {
    Log(LogDebug, frags);
  }

  GRANARY_UNUSED(env);
}

}  // namespace granary
