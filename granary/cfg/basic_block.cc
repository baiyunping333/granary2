/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/cfg/basic_block.h"
#include "granary/cfg/instruction.h"
#include "granary/metadata.h"

namespace granary {

GRANARY_DECLARE_CLASS_HEIRARCHY(
    (BasicBlock, 2),
    (NativeBasicBlock, 2 * 3),
    (InstrumentedBasicBlock, 2 * 5),
    (CachedBasicBlock, 2 * 5 * 7),
    (InFlightBasicBlock, 2 * 5 * 11),
    (FutureBasicBlock, 2 * 5 * 13),
    (UnknownBasicBlock, 2 * 5 * 17))

GRANARY_DEFINE_BASE_CLASS(BasicBlock)
GRANARY_DEFINE_DERIVED_CLASS_OF(BasicBlock, NativeBasicBlock)
GRANARY_DEFINE_DERIVED_CLASS_OF(BasicBlock, InstrumentedBasicBlock)
GRANARY_DEFINE_DERIVED_CLASS_OF(BasicBlock, CachedBasicBlock)
GRANARY_DEFINE_DERIVED_CLASS_OF(BasicBlock, InFlightBasicBlock)
GRANARY_DEFINE_DERIVED_CLASS_OF(BasicBlock, FutureBasicBlock)
GRANARY_DEFINE_DERIVED_CLASS_OF(BasicBlock, UnknownBasicBlock)

namespace detail {

// Return the next successor by iterating through the instructions in the
// basic block.
namespace {
static Instruction *FindNextSuccessorInstruction(Instruction *instr) {
  for (Instruction *curr(instr->Next()); curr; curr = curr->Next()) {
    if (IsA<ControlFlowInstruction *>(curr)) {
      return curr;
    }
  }
  return nullptr;
}
}  // namespace

SuccessorBlockIterator::SuccessorBlockIterator(Instruction *instr_)
    : cursor(FindNextSuccessorInstruction(instr_)) {}

BasicBlockSuccessor SuccessorBlockIterator::operator*(void) const {
  auto cti(DynamicCast<ControlFlowInstruction *>(cursor));
  return BasicBlockSuccessor(cti, cti->TargetBlock());
}

void SuccessorBlockIterator::operator++(void) {
  cursor = FindNextSuccessorInstruction(cursor);
}

void ForwardInstructionIterator::operator++(void) {
  instr = instr->Next();
}

void BackwardInstructionIterator::operator++(void) {
  instr = instr->Previous();
}

}  // namespace detail

detail::SuccessorBlockIterator BasicBlock::Successors(void) const {
  return detail::SuccessorBlockIterator();
}

// Get this basic block's meta-data.
GenericMetaData *InstrumentedBasicBlock::MetaData(void) {
  return meta;
}

// Initialize an instrumented basic block.
InstrumentedBasicBlock::InstrumentedBasicBlock(AppProgramCounter app_start_pc_,
                                               GenericMetaData *meta_)
    : BasicBlock(app_start_pc_),
      meta(meta_) {}

// Initialize a cached basic block.
CachedBasicBlock::CachedBasicBlock(AppProgramCounter app_start_pc_,
                                   CacheProgramCounter cache_start_pc_,
                                   GenericMetaData *meta_)
    : InstrumentedBasicBlock(app_start_pc_, meta_),
      cache_start_pc(cache_start_pc_) {}

// Initialize an in-flight basic block.
InFlightBasicBlock::InFlightBasicBlock(AppProgramCounter app_start_pc_,
                                       GenericMetaData *meta_)
    : InstrumentedBasicBlock(app_start_pc_, meta_),
      first(new AnnotationInstruction(BEGIN_BASIC_BLOCK)),
      last(new AnnotationInstruction(END_BASIC_BLOCK)) {
  first->InsertAfter(std::unique_ptr<Instruction>(last));
}

// Return an iterator of the successors of a basic block.
detail::SuccessorBlockIterator InFlightBasicBlock::Successors(void) const {
  return detail::SuccessorBlockIterator(first);
}

// Return the first instruction in the basic block.
Instruction *InFlightBasicBlock::FirstInstruction(void) const {
  return first;
}

// Return the last instruction in the basic block.
Instruction *InFlightBasicBlock::LastInstruction(void) const {
  return last;
}

// Return an iterator for the instructions of the block.
detail::ForwardInstructionIterator
InFlightBasicBlock::Instructions(void) const {
  return detail::ForwardInstructionIterator(first);
}

// Return a reverse iterator for the instructions of the block.
detail::BackwardInstructionIterator
InFlightBasicBlock::ReversedInstructions(void) const {
  return detail::BackwardInstructionIterator(last);
}

// Free all of the instructions in the basic block. This is invoked by
// LocalControlFlowGraph::~LocalControlFlowGraph, as the freeing of instructions
// interacts with the ownership model of basic blocks inside of basic block
// lists.
void InFlightBasicBlock::FreeInstructionList(void) {
  for (Instruction *instr(first), *next(nullptr); instr; instr = next) {
    next = instr->Next();
    delete instr;
  }
}

}  // namespace granary
