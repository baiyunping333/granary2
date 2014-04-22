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

#include "dependencies/xxhash/hash.h"

namespace granary {
namespace {

// Repeatedly apply LCFG-wide instrumentation for every tool, where tools are
// allowed to materialize direct basic blocks into other forms of basic blocks.
static void InstrumentControlFlow(Tool *tools,
                                  BlockFactory *factory,
                                  LocalControlFlowGraph *cfg) {
  for (;; factory->MaterializeRequestedBlocks()) {
    for (auto tool : ToolIterator(tools)) {
      tool->InstrumentControlFlow(factory, cfg);
    }
    if (!factory->HasPendingMaterializationRequest()) {
      break;
    }
  }
}

// Apply LCFG-wide instrumentation for every tool.
static void InstrumentBlocks(Tool *tools, LocalControlFlowGraph *cfg) {
  for (auto tool : ToolIterator(tools)) {
    tool->InstrumentBlocks(cfg);
  }
}

// Apply instrumentation to every block for every tool.
//
// Note: This applies tool-specific instrumentation for all tools to a single
//       block before moving on to the next block in the LCFG.
static void InstrumentBlock(Tool *tools, LocalControlFlowGraph *cfg) {
  for (auto block : cfg->Blocks()) {
    for (auto tool : ToolIterator(tools)) {
      auto decoded_block = DynamicCast<DecodedBasicBlock *>(block);
      if (decoded_block) {
        tool->InstrumentBlock(decoded_block);
      }
    }
  }
}

static uint32_t HashMetaData(BlockMetaData *meta) {
  xxhash::HashFunction hasher(0xDEADBEEFUL);
  hasher.Reset();
  meta->Hash(&hasher);
  hasher.Finalize();
  return hasher.Extract32();
}

}  // namespace

// Instrument one or more basic blocks (contained in the local control-
// flow graph, or materialized during `InstrumentControlFlow`).
void Instrument(ContextInterface *context,
                LocalControlFlowGraph *cfg,
                BlockMetaData *meta) {
  GRANARY_IF_DEBUG( auto meta_hash = ) HashMetaData(meta);

  BlockFactory factory(context, cfg);
  factory.MaterializeInitialBlock(meta);

  auto tools = context->AllocateTools();
  InstrumentControlFlow(tools, &factory, cfg);
  InstrumentBlocks(tools, cfg);
  InstrumentBlock(tools, cfg);
  context->FreeTools(tools);

  // Verify that the indexable meta-data for the entry basic block has not
  // changed during the instrumentation process.
  GRANARY_ASSERT(HashMetaData(meta) == meta_hash);
}

}  // namespace granary
