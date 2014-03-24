/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "granary/arch/x86-64/asm/include.asm.inc"

START_FILE

// Defines a function that is used to test some of the early instruction
// mangling of stack-pointer changing instructions.
DEFINE_FUNC(granary_test_mangle)
    mov %rsp, %rbp;
    sub $0x20, %rsp;

    mov %rsp, -0x8(%rbp);
    mov %rsp, (%rsp);
    mov %rsp, -0x8(%rsp);

    lea -0x8(%rsp), %rax;
    mov %rax, 0x8(%rbp);

    mov $0, %rax;

.Lloop_call_through_stack:
    mov %rsp, %rdi;
    shr $1, %rdi;
    callq *-0x8(%rsp, %rdi, 2);
    cmp $0, %rax;
    jz .Lloop_call_through_stack;

    // Pop the stack frame. This introduces some interestingness: should the
    // next fragment be considered to be on an invalid frame, or a valid one?
    // Forward propagation would make it invalid, but back-propagation from
    // the ret would make it valid.
    mov %rbp, %rsp;

.Ldecrement_rax_until_zero_stack_unsafe:
    sub $1, %rax;
    jnz .Ldecrement_rax_until_zero_stack_unsafe;

    // In this block the stack pointer should be seen as valid because of
    // the `ret`.
    mov $1, %rax;
    ret;

END_FUNC(granary_test_mangle)

END_FILE
