/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef TEST_CONTEXT_H_
#define TEST_CONTEXT_H_

#include <gmock/gmock.h>

#define GRANARY_INTERNAL

#include "granary/context.h"

// Implements a mock Granary `Context`.
class MockContext : public granary::ContextInterface {
 public:
  MockContext(void) = default;
  virtual ~MockContext(void) = default;

  // Annotates the instruction, or adds an annotated instruction into the
  // instruction list. This returns the first
  MOCK_METHOD1(AnnotateInstruction,
               void(granary::Instruction *instr));

  // Allocate and initialize some `BlockMetaData`.
  MOCK_METHOD1(AllocateBlockMetaData,
               granary::BlockMetaData *(granary::AppPC));

  // Allocate and initialize some `BlockMetaData`, based on some existing
  // meta-data template.
  MOCK_METHOD2(AllocateBlockMetaData,
               granary::BlockMetaData *(const granary::BlockMetaData *,
                                        granary::AppPC pc));

  // Allocate and initialize some empty `BlockMetaData`.
  MOCK_METHOD0(AllocateEmptyBlockMetaData,
               granary::BlockMetaData *());

  // Register some meta-data with Granary.
  MOCK_METHOD1(RegisterMetaData,
               void (const granary::MetaDataDescription ));

  // Compile some code into one of the code caches.
  MOCK_METHOD1(Compile,
               void (granary::LocalControlFlowGraph *));

  // Allocate instances of the tools that will be used to instrument blocks.
  MOCK_METHOD0(AllocateTools,
               granary::Tool *());

  // Free the allocated tools.
  MOCK_METHOD1(FreeTools,
               void (granary::Tool *tools));

  // Allocate a new code cache.
  //
  // Note: This should be a lightweight operation as it is usually invoked
  //       whilst fine-grained locks are held.
  MOCK_METHOD0(AllocateCodeCache,
               granary::CodeCacheInterface *());

  // Flush an entire code cache.
  //
  // Note: This should be a lightweight operation as it is usually invoked
  //       whilst fine-grained locks are held (e.g. schedule for the allocator
  //       to be freed).
  MOCK_METHOD1(FlushCodeCache,
               void (granary::CodeCacheInterface *));

  // Allocates a direct edge data structure, as well as the code needed to
  // back the direct edge.
  MOCK_METHOD2(AllocateDirectEdge,
               granary::DirectEdge *(const granary::BlockMetaData *,
                                     granary::BlockMetaData *));

  // Allocates a direct edge data structure, as well as the code needed to
  // back the direct edge.
  MOCK_METHOD2(AllocateIndirectEdge,
               granary::IndirectEdge *(const granary::BlockMetaData *,
                                       const granary::BlockMetaData *));

  // Get a pointer to this context's code cache index.
  MOCK_METHOD0(CodeCacheIndex, granary::LockedIndex *());

 private:
  GRANARY_DISALLOW_COPY_AND_ASSIGN(MockContext);
};

#endif  // TEST_CONTEXT_H_
