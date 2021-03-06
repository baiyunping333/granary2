/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_CODE_INLINE_ASSEMBLY_H_
#define GRANARY_CODE_INLINE_ASSEMBLY_H_

#include "granary/base/base.h"  // For `size_t`.
#include "granary/base/pc.h"  // For `AppPC`.

#ifdef GRANARY_INTERNAL
# include "granary/base/container.h"
# include "granary/base/new.h"
# include "granary/base/refcount.h"

# include "granary/cfg/operand.h"
#endif

namespace granary {

enum {
  kMaxNumInlineVars = 16
};

#ifdef GRANARY_INTERNAL

// Forward declarations.
class Trace;
class DecodedBlock;
class LabelInstruction;
class Instruction;

namespace arch {
class Operand;
}  // namespace arch

// Represents a scope of inline assembly. Within this scope, several virtual
// registers are live.
class InlineAssemblyScope : public UnownedCountedObject {
 public:
  // Initialize the input variables to the scope.
  explicit InlineAssemblyScope(std::initializer_list<const Operand *> inputs);
  virtual ~InlineAssemblyScope(void);

  GRANARY_DECLARE_NEW_ALLOCATOR(InlineAssemblyScope, {
    kAlignment = 1
  })

  // Variables used/referenced/created within the scope.
  OpaqueContainer<arch::Operand, 32, 16> vars[kMaxNumInlineVars];
  bool var_is_initialized[kMaxNumInlineVars];

 private:
  InlineAssemblyScope(void) = delete;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(InlineAssemblyScope);
};

// Represents a block of inline assembly instructions.
class InlineAssemblyBlock {
 public:
  // Initialize this block of inline assembly.
  //
  // Note: This will acquire a reference count on the scope referenced by this
  //       block.
  InlineAssemblyBlock(InlineAssemblyScope *scope_, const char *assembly_);

  // Destroy this block of inline assembly.
  //
  // Note: This will delete the associated scope iff, after releasing a
  //       reference to the scope, the scope has no more references pointing
  //       to it.
  ~InlineAssemblyBlock(void);

  GRANARY_DECLARE_NEW_ALLOCATOR(InlineAssemblyBlock, {
    kAlignment = 1
  })

  InlineAssemblyScope * const scope;
  const char * const assembly;

 private:
  InlineAssemblyBlock(void) = delete;
  GRANARY_DISALLOW_COPY_AND_ASSIGN(InlineAssemblyBlock);
};

// Represents an "inline" function call. Depending on the use, this might be
// a function that is inlined directly into the code, or where a call out to
// `target_app_pc` is added.
class InlineFunctionCall {
 public:
  InlineFunctionCall(DecodedBlock *block, AppPC target,
                     Operand ops[detail::kMaxNumFuncOperands],
                     size_t num_args_);

  inline size_t NumArguments(void) const {
    return num_args;
  }

  GRANARY_DECLARE_NEW_ALLOCATOR(InlineFunctionCall, {
    kAlignment = 1
  })

  AppPC target_app_pc;
  size_t num_args;
  Operand args[detail::kMaxNumFuncOperands];
  VirtualRegister arg_regs[detail::kMaxNumFuncOperands];

 private:
  InlineFunctionCall(void) = delete;
   GRANARY_DISALLOW_COPY_AND_ASSIGN(InlineFunctionCall);
};

#endif  // GRANARY_INTERNAL

#define GRANARY_DEFINE_ASM_OP(arch, param, ret) \
  inline const char *operator"" _ ## arch (const char *param, size_t) { \
    return ret; \
  }

#ifndef GRANARY_ECLIPSE
GRANARY_DEFINE_ASM_OP(x86, , nullptr)  // 32-bit x86.
GRANARY_DEFINE_ASM_OP(x86_64, lines, lines)  // 64-bit x86.
GRANARY_DEFINE_ASM_OP(arm, , nullptr)
GRANARY_DEFINE_ASM_OP(armv7, , nullptr)
GRANARY_DEFINE_ASM_OP(thumb, , nullptr)
GRANARY_DEFINE_ASM_OP(mips, , nullptr)
GRANARY_DEFINE_ASM_OP(sparc, , nullptr)
GRANARY_DEFINE_ASM_OP(ppc, , nullptr)
#endif  // GRANARY_ECLIPSE
}  // namespace granary

// Fake user defined literals, needed until this Eclipse syntax highlighting
// bug (`https://bugs.eclipse.org/bugs/show_bug.cgi?id=379684`) is closed.
#ifdef GRANARY_ECLIPSE
# define _x86
# define _x86_64
# define _arm
# define _armv7
# define _thumb
# define _mips
# define _sparc
# define _ppc
#else
# define IF_ECLIPSE__x86
# define IF_ECLIPSE__x86_64
# define IF_ECLIPSE__arm
# define IF_ECLIPSE__armv7
# define IF_ECLIPSE__thumb
# define IF_ECLIPSE__mips
# define IF_ECLIPSE__sparc
# define IF_ECLIPSE__ppc
#endif  // GRANARY_ECLIPSE

#undef GRANARY_DEFINE_ASM_OP

#endif  // GRANARY_CODE_INLINE_ASSEMBLY_H_
