/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/cfg/basic_block.h"
#include "granary/cfg/instruction.h"
#include "granary/metadata.h"

namespace granary {
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

// Initialize an instrumented basic block.
InstrumentedBasicBlock::InstrumentedBasicBlock(
    AppProgramCounter app_start_pc_, const BasicBlockMetaData *entry_meta_)
    : BasicBlock(app_start_pc_),
      entry_meta(entry_meta_) {

  // TODO(pag): Use the entry metadata. This should involve looking into some
  //            global "pool" of meta-data for all basic blocks, and making
  //            a copy of that meta-data for this basic block. The eventual
  //            requirement should be that entry_meta is guaranteed to be non-
  //            null.
  GRANARY_UNUSED(entry_meta);
}

// Initialize a cached basic block.
CachedBasicBlock::CachedBasicBlock(AppProgramCounter app_start_pc_,
                                   CacheProgramCounter cache_start_pc_,
                                   const BasicBlockMetaData *entry_meta_)
    : InstrumentedBasicBlock(app_start_pc_, entry_meta_),
      cache_start_pc(cache_start_pc_) {}

// Initialize an in-flight basic block.
InFlightBasicBlock::InFlightBasicBlock(AppProgramCounter app_start_pc_,
                                       const BasicBlockMetaData *entry_meta_)
    : InstrumentedBasicBlock(app_start_pc_, entry_meta_),
      meta(entry_meta_->Copy()),
      first(new AnnotationInstruction(BEGIN_BASIC_BLOCK)),
      last(new AnnotationInstruction(END_BASIC_BLOCK)) {
  first->InsertAfter(std::unique_ptr<Instruction>(last));
}

// Return an iterator of the successors of a basic block.
detail::SuccessorBlockIterator InFlightBasicBlock::Successors(void) const {
  return detail::SuccessorBlockIterator(first);
}

// Free all of the instructions in the basic block. This is invoked by
// ControlFlowGraph::~ControlFlowGraph, as the freeing of instructions
// interacts with the ownership model of basic blocks inside of basic block
// lists.
void InFlightBasicBlock::FreeInstructionList(void) {
  for (Instruction *instr(first), *next(nullptr); instr; instr = next) {
    next = instr->Next();
    delete instr;
  }
}

}  // namespace granary
