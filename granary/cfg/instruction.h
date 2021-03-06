/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_CFG_INSTRUCTION_H_
#define GRANARY_CFG_INSTRUCTION_H_

#include "granary/base/base.h"
#include "granary/base/cast.h"
#include "granary/base/list.h"
#include "granary/base/new.h"
#include "granary/base/pc.h"
#include "granary/base/type_trait.h"

#ifdef GRANARY_INTERNAL
# include "arch/driver.h"
#endif

#include "granary/cfg/operand.h"

namespace granary {

// Forward declarations.
class Block;
class ControlFlowInstruction;
class BlockFactory;
class Operand;
GRANARY_INTERNAL_DEFINITION class Fragment;
GRANARY_INTERNAL_DEFINITION enum alignas(16) UntypedData : uint64_t {
};

// Represents an abstract instruction.
class Instruction {
 public:
  GRANARY_INTERNAL_DEFINITION
  inline Instruction(void)
      : list(),
        transient_meta() {}

  virtual ~Instruction(void) = default;

  GRANARY_DECLARE_BASE_CLASS(Instruction)

  Instruction *Next(void);
  Instruction *Previous(void);

  // Used to get around issues with Eclipse's indexer.
#ifdef GRANARY_ECLIPSE
  UntypedData MetaData(void);  // Fake, non-const.
  template <typename T> T MetaData(void) const;
  template <typename T> void SetMetaData(T);
#else
  // Get the transient, tool-specific instruction meta-data as an arbitrary,
  // `uint64_t`-sized type.
  template <
    typename T,
    typename EnableIf<!TypesAreEqual<T, uint64_t>::RESULT>::Type=0
  >
  inline T MetaData(void) const {
    GRANARY_INTERNAL_DEFINITION
    static_assert(sizeof(uint64_t) >= sizeof(T),
        "Transient meta-data type is too big. Client tools can only store "
        "a `uint64_t`-sized object as meta-data inside of an instruction.");
    return UnsafeCast<T>(MetaData());
  }

  // Get the transient, tool-specific instruction meta-data as a `uintptr_t`.
  uint64_t MetaData(void) const;

  // Set the transient, tool-specific instruction meta-data as an arbitrary,
  // `uint64_t`-sized type.
  template <
    typename T,
    typename EnableIf<!TypesAreEqual<T, uint64_t>::RESULT>::Type=0
  >
  inline void SetMetaData(T meta) {
    GRANARY_INTERNAL_DEFINITION
    static_assert(sizeof(T) <= sizeof(uint64_t),
        "Transient meta-data type is too big. Client tools can only store "
        "a `uint64_t`-sized object as meta-data inside of an instruction.");
    return SetMetaData(UnsafeCast<uint64_t>(meta));
  }

  // Set the transient, tool-specific instruction meta-data as a `uintptr_t`.
  void SetMetaData(uint64_t meta);
#endif

  // Clear out the meta-data. This should be done by tools using instruction-
  // specific meta-data before they instrument instructions.
  inline void ClearMetaData(void) {
    SetMetaData(0UL);
  }

  // Inserts an instruction before/after the current instruction. Returns an
  // (unowned) pointer to the inserted instruction.
  virtual Instruction *InsertBefore(Instruction *);
  virtual Instruction *InsertAfter(Instruction *);

  inline Instruction *InsertBefore(std::unique_ptr<Instruction> instr) {
    return InsertBefore(instr.release());
  }

  inline Instruction *InsertAfter(std::unique_ptr<Instruction> instr) {
    return InsertAfter(instr.release());
  }

  // Unlink an instruction from an instruction list.
  GRANARY_INTERNAL_DEFINITION
  static std::unique_ptr<Instruction> Unlink(Instruction *);

  // Used to put instructions into lists.
  GRANARY_INTERNAL_DEFINITION mutable ListHead<Instruction> list;

 protected:
  // Transient, tool-specific meta-data stored in this instruction. The lifetime
  // of this meta-data is the length of a tool's instrumentation function (i.e. 
  // `InstrumentControlFlow`, `InstrumentBlocks`, or `InstrumentBlock`). This
  // meta-data is the backing storage for tools to temporarily attach small
  // amounts of data (e.g. pointers to data structures, bit sets of live regs)
  // to instructions for a short period of time. 
  GRANARY_INTERNAL_DEFINITION UntypedData transient_meta;

 private:

  GRANARY_IF_EXTERNAL( Instruction(void) = delete; )
  GRANARY_DISALLOW_COPY_AND_ASSIGN(Instruction);
};

// Represents a basic list of instructions. These are internal typedefs,
// mostly used later during the code assembly process.
GRANARY_INTERNAL_DEFINITION
typedef ListOfListHead<Instruction> InstructionList;

GRANARY_INTERNAL_DEFINITION
typedef ListHeadIterator<Instruction> InstructionListIterator;

GRANARY_INTERNAL_DEFINITION
typedef ReverseListHeadIterator<Instruction> ReverseInstructionListIterator;

// Built-in annotations.
GRANARY_INTERNAL_DEFINITION
enum InstructionAnnotation {
  // Used when we "kill" off meaningful annotations but want to leave the
  // associated instructions around.
  kAnnotNoOp,

  // Dummy annotations representing the beginning and end of a given basic
  // block.
  kAnnotBeginBlock,
  kAnnotEndBlock,

  // The code that follows this code is "cold". The value of this annotation
  // is a `CodeCacheKind`.
  kAnnotationCodeCacheKind,

  // Target of a branch instruction.
  kAnnotationLabel,

  // Marks the stack as changing to a valid or undefined stack pointer value.
  kAnnotInvalidStack,

  // Save and restore instructions for a register into a slot. The data
  // associated with this annotation is a `VirtualRegister`.
  //
  // Note: Saves and restores only operate on architectural GPRs.
  kAnnotSaveRegister,
  kAnnotRestoreRegister,
  kAnnotSwapRestoreRegister,

  // Force some registers to be live. This is useful for specifying that the
  // native values of some arch GPRs *must* be passed to something like an
  // inline call.
  //
  // The data associated with this annotation is a `UsedRegisterSet`.
  kAnnotReviveRegisters,

  // Inject a "late" stack switch instruction if the stack is not safe. Before
  // we do the stack analysis, we can realize that some things (e.g. inline/
  // context calls) might need to swap stacks, but not necessarily. These are
  // a hint for injecting stack switches later when we have verified things.
  kAnnotCondLeaveNativeStack,
  kAnnotCondEnterNativeStack,

  // An annotation that, when encoded, updates the value of some pointer with
  // the encoded address.
  //
  // The data associated with this annotation is a pointer to a pointer.
  kAnnotUpdateAddressWhenEncoded,

  // Represents a point between two logical instructions. This exists to
  // document the logical boundaries between native instructions.
  kAnnotLogicalInstructionBoundary,

  // Represents a definite change in the interrupt delivery state. If this
  // happens then we must break a fragment and isolate the instruction that
  // is changing the interrupt state.
  kAnnotInterruptDeliveryStateChange,

  // Represents an inline assembly instruction.
  //
  // The data associated with this instruction is a pointer to a
  // `InlineAssemblyBlock`.
  kAnnotInlineAssembly,

  // Represents a call to a client function that saves and restores the
  // entire machine context.
  //
  // The data associated with this annotation is a function pointer.
  kAnnotContextFunctionCall,

  // Represents a call to a client function that passes along some arguments
  // as well.
  //
  // The data associated with this annotation is a pointer to a
  // `InlineFunctionCall`.
  kAnnotInlineFunctionCall
};

// An annotation instruction is an environment-specific and implementation-
// specific annotations for basic blocks. Some examples would be that some
// instructions might result in page faults within kernel code. Annotations
// are used to mark those boundaries (e.g. by having an annotation that begins
// a faultable sequence of instructions and an annotation that ends it).
// Annotation instructions should not be removed by instrumentation.
class AnnotationInstruction : public Instruction {
 public:
  virtual ~AnnotationInstruction(void);

  GRANARY_INTERNAL_DEFINITION
  inline explicit AnnotationInstruction(InstructionAnnotation annotation_)
      : annotation(annotation_),
        data() {}

  GRANARY_INTERNAL_DEFINITION
  template <typename T>
  inline AnnotationInstruction(InstructionAnnotation annotation_,
                               T data_)
      : annotation(annotation_),
        data(UnsafeCast<UntypedData>(data_)) {}

  virtual Instruction *InsertBefore(Instruction *) override;
  virtual Instruction *InsertAfter(Instruction *) override;

  // Returns true if this instruction is a label.
  bool IsLabel(void) const;

  // Returns true if this instruction is targeted by any branches.
  bool IsBranchTarget(void) const;

  // Returns true if this represents the beginning of a new logical instruction.
  bool IsInstructionBoundary(void) const;

  GRANARY_INTERNAL_DEFINITION GRANARY_CONST
  InstructionAnnotation annotation;

  GRANARY_INTERNAL_DEFINITION GRANARY_CONST
  UntypedData data;

  GRANARY_DECLARE_DERIVED_CLASS_OF(Instruction, AnnotationInstruction)
  GRANARY_DECLARE_INTERNAL_NEW_ALLOCATOR(AnnotationInstruction, {
    kAlignment = 1
  })

  GRANARY_INTERNAL_DEFINITION
  template <typename T>
  inline T Data(void) const {
    return UnsafeCast<T>(data);
  }

  GRANARY_INTERNAL_DEFINITION
  template <typename T>
  inline void SetData(T new_data) {
    static_assert(sizeof(T) <= sizeof(uintptr_t),
                  "Cannot store data inside of annotation.");
    data = UnsafeCast<UntypedData>(new_data);
  }

  GRANARY_INTERNAL_DEFINITION
  template <typename T>
  inline T *DataPtr(void) {
    static_assert(sizeof(T) <= sizeof(uintptr_t),
                  "Cannot store data inside of annotation.");
    return UnsafeCast<T *>(&data);
  }

  GRANARY_INTERNAL_DEFINITION
  template <typename T>
  inline T &DataRef(void) {
    static_assert(sizeof(T) <= sizeof(uintptr_t),
                  "Cannot store data inside of annotation.");
    return *UnsafeCast<T *>(&data);
  }

 private:
  AnnotationInstruction(void) = delete;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(AnnotationInstruction);
};

// A label instruction. Just a specialized annotation instruction. Enforces at
// the type level that local control-flow instructions (within a block) must
// target a label. This makes it easier to identify fragment heads down the
// line when doing register allocation and assembling.
class LabelInstruction final : public AnnotationInstruction {
 public:
  LabelInstruction(void);

  GRANARY_DECLARE_DERIVED_CLASS_OF(Instruction, LabelInstruction)
  GRANARY_DECLARE_NEW_ALLOCATOR(LabelInstruction, {
    kAlignment = 1
  })

  GRANARY_INTERNAL_DEFINITION Fragment *fragment;
};

// An instruction containing an architecture-specific decoded instruction.
class NativeInstruction : public Instruction {
 public:
  virtual ~NativeInstruction(void);

  GRANARY_INTERNAL_DEFINITION
  explicit NativeInstruction(const arch::Instruction *instruction_);

  // Return the address in the native code from which this instruction was
  // decoded.
  //
  // Note: Granary might expand a single instruction into many instructions. In
  //       these cases, multiple instructions might have the same `DecodedPC`
  //       value. In practice, the only instructions in such cases that will be
  //       treated as application instructions (thus having a decoded PC) will
  //       be those related to operations on the *explicit* operands of the
  //       instruction.
  AppPC DecodedPC(void) const;

  // Get the decoded length of the instruction. This is independent from the
  // length of the encoded instruction, which could be wildly different as a
  // single decoded instruction might map to many encoded instructions. If the
  // instruction was not decoded then this returns 0.
  size_t DecodedLength(void) const;

  // Returns the total number of operands.
  size_t NumOperands(void) const;

  // Returns the total number of explicit operands.
  size_t NumExplicitOperands(void) const;

  // Returns true if this instruction is essentially a no-op, i.e. it does
  // nothing and has no observable side-effects.
  bool IsNoOp(void) const;

  // Driver-specific implementations.
  bool ReadsConditionCodes(void) const;
  bool WritesConditionCodes(void) const;
  bool IsFunctionCall(void) const;
  bool IsFunctionTailCall(void) const;  // A call converted into a jump.
  bool IsFunctionReturn(void) const;
  bool IsInterruptCall(void) const;
  bool IsInterruptReturn(void) const;
  bool IsSystemCall(void) const;
  bool IsSystemReturn(void) const;
  bool IsJump(void) const;
  bool IsUnconditionalJump(void) const;
  bool IsConditionalJump(void) const;
  virtual bool HasIndirectTarget(void) const;

  // Does this instruction perform an atomic read/modify/write?
  bool IsAtomic(void) const;

  // Note: See `NativeInstruction::DecodedPC` for some details related to
  //       how native instructions might be decoded into many instructions, not
  //       all of which necessarily have a non-NULL `DecodedPC`.
  bool IsAppInstruction(void) const;

  GRANARY_INTERNAL_DEFINITION void MakeAppInstruction(PC decoded_pc);
  GRANARY_INTERNAL_DEFINITION bool ReadsFromStackPointer(void) const;
  GRANARY_INTERNAL_DEFINITION bool WritesToStackPointer(void) const;

  // Get the opcode name. The opcode name of an instruction is a semantic
  // name that conveys the meaning of the instruction, but not necessarily
  // any particular encoding of the instruction. For example, many different
  // instructions that achieve the same semantic goal can have the same opcode
  // name because they operate on different kinds or numbers of operands.
  const char *OpCodeName(void) const;

  // Get the instruction selection name. The instruction selection name is as
  // close to an unambiguous name for the instruction as we can get. It should
  // make it obvious which encoding will be used for this instruction, which
  // type of operands this instructions takes, etc.
  const char *ISelName(void) const;

  // Returns the names of the instruction prefixes on this instruction.
  const char *PrefixNames(void) const;

  // Try to match and bind one or more operands from this instruction.
  //
  // Note: Matches are attempted in order!
  template <typename... OperandMatchers>
  inline bool MatchOperands(OperandMatchers... matchers) {
    return sizeof...(matchers) == CountMatchedOperandsImpl({matchers...});
  }

  // Try to match and bind one or more operands from this instruction. Returns
  // the number of operands matched, starting from the first operand.
  template <typename... OperandMatchers>
  inline size_t CountMatchedOperands(OperandMatchers... matchers) {
    return CountMatchedOperandsImpl({matchers...});
  }

  // Invoke a function on every operand.
  template <typename FuncT>
  inline void ForEachOperand(FuncT func) {
    ForEachOperandImpl(std::cref(func));
  }

  GRANARY_DECLARE_DERIVED_CLASS_OF(Instruction, NativeInstruction)
  GRANARY_DECLARE_INTERNAL_NEW_ALLOCATOR(NativeInstruction, {
    kAlignment = 1
  })

 GRANARY_ARCH_PUBLIC:
  // Mid-level IR that represents the instruction.
  GRANARY_INTERNAL_DEFINITION arch::Instruction instruction;

  // IDs of VRs defined and used by this instruction.
  GRANARY_INTERNAL_DEFINITION unsigned num_used_vrs;
  GRANARY_INTERNAL_DEFINITION uint16_t defined_vr;
  GRANARY_INTERNAL_DEFINITION uint16_t used_vrs[4];

  // OS-specific annotation for this instruction. For example, in the Linux
  // kernel, this would be an exception table entry.
  GRANARY_INTERNAL_DEFINITION const void *os_annotation;

 private:
  friend class ControlFlowInstruction;

  // Invoke a function on every operand.
  void ForEachOperandImpl(const std::function<void(Operand *)> &func);

  NativeInstruction(void) = delete;

  // Try to match and bind one or more operands from this instruction. Returns
  // the number of operands matched, starting from the first operand.
  size_t CountMatchedOperandsImpl(
      std::initializer_list<OperandMatcher> matchers);

  GRANARY_DISALLOW_COPY_AND_ASSIGN(NativeInstruction);
};

// Represents a control-flow instruction that is local to a basic block, i.e.
// keeps control within the same basic block.
class BranchInstruction final : public NativeInstruction {
 public:
  virtual ~BranchInstruction(void) = default;

  GRANARY_INTERNAL_DEFINITION
  BranchInstruction(const arch::Instruction *instruction_,
                    AnnotationInstruction *target_);

  // Return the targeted instruction of this branch.
  LabelInstruction *TargetLabel(void) const;

  // Modify this branch to target a different label.
  GRANARY_INTERNAL_DEFINITION
  void SetTargetInstruction(LabelInstruction *label);

  GRANARY_DECLARE_DERIVED_CLASS_OF(Instruction, BranchInstruction)
  GRANARY_DECLARE_INTERNAL_NEW_ALLOCATOR(BranchInstruction, {
    kAlignment = 1
  })

 private:
  BranchInstruction(void) = delete;

  // Instruction targeted by this branch. Assumed to be within the same
  // basic block as this instruction.
  GRANARY_INTERNAL_DEFINITION LabelInstruction * GRANARY_CONST target;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(BranchInstruction);
};

// Represents a control-flow instruction that is not local to a basic block,
// i.e. transfers control to another basic block.
//
// Note: A special case is that a non-local control-flow instruction can
//       redirect control back to the beginning of the basic block.
class ControlFlowInstruction : public NativeInstruction {
 public:
  virtual ~ControlFlowInstruction(void);

  GRANARY_INTERNAL_DEFINITION
  ControlFlowInstruction(const arch::Instruction *instruction_,
                         Block *target_);

  virtual bool HasIndirectTarget(void) const;

  // Return the target block of this CFI.
  Block *TargetBlock(void) const;

  GRANARY_DECLARE_DERIVED_CLASS_OF(Instruction, ControlFlowInstruction)
  GRANARY_DECLARE_INTERNAL_NEW_ALLOCATOR(ControlFlowInstruction, {
    kAlignment = 1
  })

 private:
  friend class BlockFactory;

  ControlFlowInstruction(void) = delete;

  // Target block of this CFI.
  GRANARY_INTERNAL_DEFINITION mutable Block *target;

  GRANARY_INTERNAL_DEFINITION
  void ChangeTarget(Block *new_target) const;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(ControlFlowInstruction);
};

// Represents an exceptional control-flow instruction. These are instructions
// that the OS expects to fault and handles in a specific way that doesn't
// necessarily return execution to the next instruction.
class ExceptionalControlFlowInstruction : public ControlFlowInstruction {
 public:
  GRANARY_INTERNAL_DEFINITION
  ExceptionalControlFlowInstruction(const arch::Instruction *instruction_,
                                    const arch::Instruction *orig_instruction_,
                                    Block *exception_target_,
                                    AppPC emulation_pc_);

  GRANARY_DECLARE_DERIVED_CLASS_OF(Instruction,
                                   ExceptionalControlFlowInstruction)
  GRANARY_DECLARE_INTERNAL_NEW_ALLOCATOR(ExceptionalControlFlowInstruction, {
    kAlignment = 1
  })

 GRANARY_PROTECTED:
  // Decoded instruction before it was mangled.
  GRANARY_INTERNAL_DEFINITION arch::Instruction orig_instruction;

  // Used registers in the instruction, before any mangling might have
  // happened. These are the registers that absolutely must be saved before
  // the instruction executes and restored if the instruction faults and
  // doesn't return to the faulting PC.
  GRANARY_INTERNAL_DEFINITION UsedRegisterSet used_regs;

  // This is the set of registers that we use to restore the regs from
  // `used_regs`.
  GRANARY_INTERNAL_DEFINITION
  VirtualRegister saved_regs[arch::NUM_GENERAL_PURPOSE_REGISTERS];

  // Internal Granary code that handles emulating the original instruction.
  GRANARY_INTERNAL_DEFINITION AppPC emulation_pc;

 private:
  ExceptionalControlFlowInstruction(void) = delete;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(ExceptionalControlFlowInstruction);
};

}  // namespace granary

#endif  // GRANARY_CFG_INSTRUCTION_H_
