/* Copyright 2014 Peter Goodman, all rights reserved. */

#include <granary.h>

using namespace granary;

// Tool that implements several user-space special cases for instrumenting
// common binaries.
class UserSpaceInstrumenter : public InstrumentationTool {
 public:
  virtual ~UserSpaceInstrumenter(void) = default;
  virtual void InstrumentControlFlow(BlockFactory *factory,
                                     LocalControlFlowGraph *cfg) {
    for (auto block : cfg->NewBlocks()) {
      auto direct_block = DynamicCast<DirectBasicBlock *>(block);
      if (!direct_block) continue;

      // If this block targets `libdl` or `libld` then detach.
      auto module = ModuleContainingPC(direct_block->StartAppPC());
      if (StringsMatch("dl", module->Name()) ||
          StringsMatch("ld", module->Name())) {
        factory->RequestBlock(direct_block, BlockRequestKind::REQUEST_NATIVE);
      }
    }
  }
};

// Initialize the `user` tool.
GRANARY_CLIENT_INIT({
  RegisterInstrumentationTool<UserSpaceInstrumenter>("user");
})
