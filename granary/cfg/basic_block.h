/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef GRANARY_CFG_BASIC_BLOCK_H_
#define GRANARY_CFG_BASIC_BLOCK_H_

#include "granary/base/base.h"
#include "granary/base/cast.h"
#include "granary/base/type_traits.h"

namespace granary {

class Instruction;


GRANARY_DECLARE_CLASS_HEIRARCHY(
    BasicBlock,
    CachedBasicBlock,
    InFlightBasicBlock,
    FutureBasicBlock,
    UnknownBasicBlock);


// Forward declarations.
class BasicBlock;
class ControlFlowGraph;


// Iterator for basic blocks.
class BasicBlockIterator {
 public:
  explicit BasicBlockIterator(const BasicBlock *);

  void Reset(void);

 private:
  BasicBlockIterator(void) = delete;
  GRANARY_DISALLOW_COPY_AND_ASSIGN(BasicBlockIterator);
};


// Abstract basic block of instructions.
class BasicBlock {
 public:
  virtual ~BasicBlock(void) = default;
  virtual bool NextSuccessor(BasicBlockIterator &, BasicBlock *&) = 0;
  virtual bool NextPredecessor(BasicBlockIterator &, BasicBlock *&) = 0;

  GRANARY_BASE_CLASS(BasicBlock)

 private:
  GRANARY_DISALLOW_COPY_AND_ASSIGN(BasicBlock);
};


// A basic block that has already been committed to the code cache.
class CachedBasicBlock : public BasicBlock {
 public:
  virtual ~CachedBasicBlock(void) = default;

  GRANARY_DERIVED_CLASS_OF(BasicBlock, CachedBasicBlock)

 private:
  // If a basic block is committed to the cache, then it typically has either
  // one or two successors: one if there is a fall-through or direct jump to
  // another block, and two if the basic block ends in a conditional jump that
  // falls through to another basic block.
  BasicBlock *successors[2];

  GRANARY_DISALLOW_COPY_AND_ASSIGN(CachedBasicBlock);
};


// A basic block that has been decoded but not yet committed to the code cache.
class InFlightBasicBlock : public BasicBlock {
 public:
  virtual ~InFlightBasicBlock(void) = default;

  GRANARY_DERIVED_CLASS_OF(BasicBlock, InFlightBasicBlock)

 private:
  GRANARY_DISALLOW_COPY_AND_ASSIGN(InFlightBasicBlock);
};


// A basic block that has not yet been decoded, and might eventually be decoded.
class FutureBasicBlock : public BasicBlock {
 public:
  virtual ~FutureBasicBlock(void) = default;

  //
  void Materialize(ControlFlowGraph &cfg, BasicBlockIterator &blocks);

  GRANARY_DERIVED_CLASS_OF(BasicBlock, FutureBasicBlock)

 private:
  GRANARY_DISALLOW_COPY_AND_ASSIGN(FutureBasicBlock);
};


// A basic block that has not yet been decoded, and which we don't know about
// at this time because it's the target of an indirect jump/call.
class UnknownBasicBlock : public BasicBlock {
 public:
  virtual ~UnknownBasicBlock(void) = default;

  GRANARY_DERIVED_CLASS_OF(BasicBlock, UnknownBasicBlock)

 private:
  GRANARY_DISALLOW_COPY_AND_ASSIGN(UnknownBasicBlock);
};


}  // namespace granary

#endif  // GRANARY_CFG_BASIC_BLOCK_H_