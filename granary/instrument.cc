/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/base/base.h"

#include "granary/cfg/control_flow_graph.h"
#include "granary/cfg/basic_block.h"
#include "granary/cfg/factory.h"

#include "granary/instrument.h"

#include "granary/breakpoint.h"
#include "granary/context.h"
#include "granary/metadata.h"
#include "granary/tool.h"

namespace granary {

// Initialize a binary instrumenter.
BinaryInstrumenter::BinaryInstrumenter(Context *context_,
                                       LocalControlFlowGraph *cfg_,
                                       BlockMetaData **meta_)
    : context(context_),
      tools(context->AllocateTools()),
      meta(meta_),
      cfg(cfg_),
      factory(context, cfg) {}

BinaryInstrumenter::~BinaryInstrumenter(void) {
  context->FreeTools(tools);
}

// Instrument some code as-if it is targeted by a direct CFI.
void BinaryInstrumenter::InstrumentDirect(void) {
  auto entry_block = factory.RequestDirectEntryBlock(meta);
  if (!entry_block) {  // Couldn't find or adapt to a existing block.
    entry_block = factory.MaterializeDirectEntryBlock(*meta);
  }

  *meta = nullptr;  // Potentially undefined after this point.

  if (IsA<DecodedBasicBlock *>(entry_block)) {  // Instrument decoded blocks.
    InstrumentControlFlow();
    InstrumentBlocks();
    InstrumentBlock();
  }

  *meta = entry_block->UnsafeMetaData();
}

// Instrument some code as-if it is targeted by an indirect CFI.
void BinaryInstrumenter::InstrumentIndirect(void) {
  factory.MaterializeIndirectEntryBlock(*meta);
  InstrumentControlFlow();
  InstrumentBlocks();
  InstrumentBlock();
}

// Instrument some code as-if it is targeted by a native entrypoint. These
// are treated as being the initial points of instrumentation.
void BinaryInstrumenter::InstrumentEntryPoint(EntryPointKind kind,
                                              int category) {
  factory.MaterializeIndirectEntryBlock(*meta);
  auto entry_block = DynamicCast<CompensationBasicBlock *>(cfg->EntryBlock());
  for (auto tool : ToolIterator(tools)) {
    tool->InstrumentEntryPoint(&factory, entry_block, kind, category);
  }
  if (factory.HasPendingMaterializationRequest()) {
    factory.MaterializeRequestedBlocks();
  }
  InstrumentControlFlow();
  InstrumentBlocks();
  InstrumentBlock();
}

namespace {

// Try to finalize the control-flow bt converting any remaining
// `DirectBasicBlock`s into `CachedBasicBlock`s (which are potentially preceded
// by `CompensationBasicBlock`.
static bool FinalizeControlFlow(BlockFactory *factory,
                                LocalControlFlowGraph *cfg) {
  for (auto block : cfg->Blocks()) {
    for (auto succ : block->Successors()) {
      factory->RequestBlock(succ.block, kRequestBlockFromIndexOrCFGOnly);
    }
  }
  return factory->HasPendingMaterializationRequest();
}

}  // namespace

// Repeatedly apply LCFG-wide instrumentation for every tool, where tools are
// allowed to materialize direct basic blocks into other forms of basic
// blocks.
void BinaryInstrumenter::InstrumentControlFlow(void) {
  for (auto finalized = false; ; factory.MaterializeRequestedBlocks()) {
    for (auto tool : ToolIterator(tools)) {
      tool->InstrumentControlFlow(&factory, cfg);
    }
    if (!factory.HasPendingMaterializationRequest()) {
      if (finalized) {
        return;

      // Try to force one more round of control-flow requests so that we can
      // submit requests to look into the code cache index.
      } else {
        finalized = true;
        if (!FinalizeControlFlow(&factory, cfg)) return;
      }
    } else {
      finalized = false;
    }
  }
}

// Apply LCFG-wide instrumentation for every tool.
void BinaryInstrumenter::InstrumentBlocks(void) {
  for (auto tool : ToolIterator(tools)) {
    tool->InstrumentBlocks(cfg);
  }
}

// Apply instrumentation to every block for every tool.
//
// Note: This applies tool-specific instrumentation for all tools to a single
//       block before moving on to the next block in the LCFG.
void BinaryInstrumenter::InstrumentBlock(void) {
  for (auto block : cfg->Blocks()) {
    for (auto tool : ToolIterator(tools)) {
      if (auto decoded_block = DynamicCast<DecodedBasicBlock *>(block)) {
        tool->InstrumentBlock(decoded_block);
      }
    }
  }
}

}  // namespace granary
