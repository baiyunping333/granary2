/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef GRANARY_ARCH_X86_64_MANGLE_EARLY_H_
#define GRANARY_ARCH_X86_64_MANGLE_EARLY_H_

namespace granary {

class DecodedBasicBlock;

namespace arch {

class Instruction;

// Perform "early" mangling of some instructions. This is primary to make the
// task of virtual register allocation tractable.
void MangleDecodedInstruction(DecodedBasicBlock *block, Instruction *instr);

}  // namespace arch
}  // namespace granary

#endif  // GRANARY_ARCH_X86_64_MANGLE_EARLY_H_