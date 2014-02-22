/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_OPERAND_OPERAND_H_
#define GRANARY_OPERAND_OPERAND_H_

#include "granary/operand/match.h"

namespace granary {

// Forward declarations.
class DecodedBasicBlock;
class NativeInstruction;

#ifdef GRANARY_INTERNAL
namespace driver {
// Forward declarations.
class Instruction;
class Operand;
}  // namespace driver
#endif  // GRANARY_INTERNAL

// A generic operand to a native instruction.
class Operand {
 public:
  bool IsMemoryOperand(void) const;
  bool IsRegisterOperand(void) const;

  // Driver-specific implementations.
  bool IsRead(void) const;
  bool IsWrite(void) const;
  bool IsConditionalRead(void) const;
  bool IsConditionalWrite(void) const;
  int Width(void) const;

  // Conveniences.
  inline bool IsReadWrite(void) const {
    return IsRead() && IsWrite();
  }

 GRANARY_PROTECTED:
  Operand(void) GRANARY_EXTERNAL_DELETE;

  // The native instruction to which this operand belongs.
  GRANARY_INTERNAL_DEFINITION NativeInstruction *native_instr;

  // The driver instruction to which this operand belongs.
  GRANARY_INTERNAL_DEFINITION driver::Instruction *driver_instr;

  // The native operand to which this operand refers, if its a reference.
  GRANARY_INTERNAL_DEFINITION driver::Operand *driver_op;

  // The kind of this memory operand.
  GRANARY_INTERNAL_DEFINITION enum {
    OP_UNDEFINED,
    OP_MEMORY,
    OP_NATIVE_REGISTER,
    OP_VIRTUAL_REGISTER
  } kind;

  // TODO(pag): Might need some stronger semblance of a "barrier" instruction
  //            so that computed addresses and registers in progress aren't
  //            committed to instructions eagerly.
};

class MemoryOperand : Operand {

};

class RegisterOperand : Operand {
 public:
  bool IsNative(void) const;
  bool IsVirtual(void) const;

  // TODO(pag): Overload operators to get memory operands from this register?
  //            Need to think about what it would be like to do something like:
  //
  //                MemoryOperand mloc1;
  //                auto addr1 = EffectiveAddress(block, mloc1);
  //                auto mloc2 = addr[10];
  //                auto addr2 = EffectiveAddress(block, mloc2);
  //
  //            Solution might be to have an intermediate object representing
  //            an l-value mloc. It can be used anywhere that a MemoryOperand
  //            is acceptable, but only commits an operation when placed in an
  //            instruction.
};

class ImmediateOperand : Operand {
 public:
};

// Returns an operand matcher against an operand that is read.
inline static detail::OperandMatcher ReadFrom(Operand &op) {
  return {&op, detail::OperandAction::READ, false};
}

// Returns an operand matcher against an operand that is written.
inline static detail::OperandMatcher WriteTo(Operand &op) {
  return {&op, detail::OperandAction::WRITE, false};
}

// Returns an operand matcher against an operand that is read and written.
inline static detail::OperandMatcher ReadAndWriteTo(Operand &op) {
  return {&op, detail::OperandAction::READ_WRITE, false};
}

// Returns an operand matcher against an operand that is read and written.
inline static detail::OperandMatcher ReadOrWriteTo(Operand &op) {
  return {&op, detail::OperandAction::ANY, false};
}

// Returns the effective address for a memory operand. The returned operand
// will either be a native or virtual register.
RegisterOperand GetEffectiveAddress(DecodedBasicBlock *block,
                                    MemoryOperand op);

}  // namespace granary

#endif  // GRANARY_OPERAND_OPERAND_H_
