/* Copyright 2014 Peter Goodman, all rights reserved. */

#include <gmock/gmock.h>

#define GRANARY_INTERNAL
#define GRANARY_ARCH_INTERNAL

#include "test/util/simple_encoder.h"

#include "granary/cfg/control_flow_graph.h"

#include "granary/code/compile.h"
#include "granary/code/edge.h"
#include "granary/code/metadata.h"

#include "granary/index.h"
#include "granary/instrument.h"
#include "granary/util.h"

using namespace granary;
using namespace testing;

SimpleEncoderTest::SimpleEncoderTest(void)
    : module(ModuleKind::GRANARY, "granary"),
      code_cache_mod(ModuleKind::GRANARY_CODE_CACHE, "[code cache]"),
      edge_cache_mod(ModuleKind::GRANARY_CODE_CACHE, "[edge cache]"),
      code_cache(&code_cache_mod, 1),
      edge_cache(&edge_cache_mod, 1),
      index(new MockIndex),
      locked_index(index) {
  meta_manager.Register<AppMetaData>();
  meta_manager.Register<CacheMetaData>();
  meta_manager.Register<LiveRegisterMetaData>();
  meta_manager.Register<StackMetaData>();
  meta_manager.Register<IndexMetaData>();
  arch::Init();
  module.SetContext(&context);
  module.AddRange(0, ~0ULL, 0, MODULE_EXECUTABLE);

  // Called for the "lazy" meta-data on the function return.
  EXPECT_CALL(context, BlockCodeCache())
      .Times(1)
      .WillOnce(Return(&code_cache));
}

BlockMetaData *SimpleEncoderTest::AllocateMeta(AppPC pc) {
  auto meta = meta_manager.Allocate();
  MetaDataCast<AppMetaData *>(meta)->start_pc = pc;
  return meta;
}

CachePC SimpleEncoderTest::InstrumentAndEncode(AppPC pc) {
  using namespace granary;
  LocalControlFlowGraph cfg(&context);

  auto meta = AllocateMeta(pc);

  EXPECT_CALL(context, CodeCacheIndex())
      .WillRepeatedly(Return(&locked_index));

  EXPECT_CALL(*index, Request(meta))
      .WillOnce(Return(IndexFindResponse{UnificationStatus::REJECT, nullptr}));

  // Called for the "lazy" meta-data on the function return.
  EXPECT_CALL(context, AllocateEmptyBlockMetaData())
      .Times(1)
      .WillOnce(InvokeWithoutArgs([&] {
        return meta_manager.Allocate();
      }));

  // Allocate all tools to instrument the first block.
  EXPECT_CALL(context, AllocateTools())
      .Times(1)
      .WillOnce(Return(nullptr));

  // Free all tools after instrumenting the LCFG.
  EXPECT_CALL(context, FreeTools(nullptr))
      .Times(1);

  Instrument(&context, &cfg, meta);
  Compile(&context, &cfg);

  auto block = cfg.EntryBlock();
  auto cache_meta = GetMetaData<CacheMetaData>(block);

  return cache_meta->start_pc;
}
