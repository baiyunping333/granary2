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
      index(new MockIndex),
      locked_index(index) {
  meta_manager.Register<ModuleMetaData>();
  meta_manager.Register<CacheMetaData>();
  meta_manager.Register<LiveRegisterMetaData>();
  meta_manager.Register<StackMetaData>();
  meta_manager.Register<IndexMetaData>();
  arch::Init();

  // Called for the "lazy" meta-data on the function return.
  EXPECT_CALL(context, AllocateCodeCache())
      .Times(1)
      .WillOnce(Return(&code_cache));

  module.SetContext(&context);
  module.AddRange(0, ~0ULL, 0, MODULE_EXECUTABLE);
}

SimpleEncoderTest::~SimpleEncoderTest(void) {
  // Called for the "lazy" meta-data on the function return.
  EXPECT_CALL(context, FlushCodeCache(&code_cache))
      .Times(1);
}

BlockMetaData *SimpleEncoderTest::AllocateMeta(AppPC pc) {
  auto meta = meta_manager.Allocate();
  auto module_meta = MetaDataCast<ModuleMetaData *>(meta);
  module_meta->start_pc = pc;
  module_meta->source.module = &module;
  module_meta->source.offset = 0;
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
  Compile(&cfg);

  auto block = cfg.EntryBlock();
  auto cache_meta = GetMetaData<CacheMetaData>(block);

  return cache_meta->cache_pc;
}