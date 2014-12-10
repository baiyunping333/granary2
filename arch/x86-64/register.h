/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef ARCH_X86_64_REGISTER_H_
#define ARCH_X86_64_REGISTER_H_

#include "granary/code/register.h"

namespace granary {
namespace arch {

extern const VirtualRegister REG_AX;
extern const VirtualRegister REG_CX;
extern const VirtualRegister REG_DX;
extern const VirtualRegister REG_BX;
extern const VirtualRegister REG_SP;
extern const VirtualRegister REG_BP;
extern const VirtualRegister REG_SI;
extern const VirtualRegister REG_DI;
extern const VirtualRegister REG_R8W;
extern const VirtualRegister REG_R9W;
extern const VirtualRegister REG_R10W;
extern const VirtualRegister REG_R11W;
extern const VirtualRegister REG_R12W;
extern const VirtualRegister REG_R13W;
extern const VirtualRegister REG_R14W;
extern const VirtualRegister REG_R15W;
extern const VirtualRegister REG_EAX;
extern const VirtualRegister REG_ECX;
extern const VirtualRegister REG_EDX;
extern const VirtualRegister REG_EBX;
extern const VirtualRegister REG_ESP;
extern const VirtualRegister REG_EBP;
extern const VirtualRegister REG_ESI;
extern const VirtualRegister REG_EDI;
extern const VirtualRegister REG_R8D;
extern const VirtualRegister REG_R9D;
extern const VirtualRegister REG_R10D;
extern const VirtualRegister REG_R11D;
extern const VirtualRegister REG_R12D;
extern const VirtualRegister REG_R13D;
extern const VirtualRegister REG_R14D;
extern const VirtualRegister REG_R15D;
extern const VirtualRegister REG_RAX;
extern const VirtualRegister REG_RCX;
extern const VirtualRegister REG_RDX;
extern const VirtualRegister REG_RBX;
extern const VirtualRegister REG_RSP;
extern const VirtualRegister REG_RBP;
extern const VirtualRegister REG_RSI;
extern const VirtualRegister REG_RDI;
extern const VirtualRegister REG_R8;
extern const VirtualRegister REG_R9;
extern const VirtualRegister REG_R10;
extern const VirtualRegister REG_R11;
extern const VirtualRegister REG_R12;
extern const VirtualRegister REG_R13;
extern const VirtualRegister REG_R14;
extern const VirtualRegister REG_R15;
extern const VirtualRegister REG_AL;
extern const VirtualRegister REG_CL;
extern const VirtualRegister REG_DL;
extern const VirtualRegister REG_BL;
extern const VirtualRegister REG_SPL;
extern const VirtualRegister REG_BPL;
extern const VirtualRegister REG_SIL;
extern const VirtualRegister REG_DIL;
extern const VirtualRegister REG_R8B;
extern const VirtualRegister REG_R9B;
extern const VirtualRegister REG_R10B;
extern const VirtualRegister REG_R11B;
extern const VirtualRegister REG_R12B;
extern const VirtualRegister REG_R13B;
extern const VirtualRegister REG_R14B;
extern const VirtualRegister REG_R15B;
extern const VirtualRegister REG_AH;
extern const VirtualRegister REG_CH;
extern const VirtualRegister REG_DH;
extern const VirtualRegister REG_BH;
extern const VirtualRegister REG_ERROR;
extern const VirtualRegister REG_RIP;
extern const VirtualRegister REG_EIP;
extern const VirtualRegister REG_IP;
extern const VirtualRegister REG_K0;
extern const VirtualRegister REG_K1;
extern const VirtualRegister REG_K2;
extern const VirtualRegister REG_K3;
extern const VirtualRegister REG_K4;
extern const VirtualRegister REG_K5;
extern const VirtualRegister REG_K6;
extern const VirtualRegister REG_K7;
extern const VirtualRegister REG_MMX0;
extern const VirtualRegister REG_MMX1;
extern const VirtualRegister REG_MMX2;
extern const VirtualRegister REG_MMX3;
extern const VirtualRegister REG_MMX4;
extern const VirtualRegister REG_MMX5;
extern const VirtualRegister REG_MMX6;
extern const VirtualRegister REG_MMX7;
extern const VirtualRegister REG_CS;
extern const VirtualRegister REG_DS;
extern const VirtualRegister REG_ES;
extern const VirtualRegister REG_SS;
extern const VirtualRegister REG_FS;
extern const VirtualRegister REG_GS;
extern const VirtualRegister REG_ST0;
extern const VirtualRegister REG_ST1;
extern const VirtualRegister REG_ST2;
extern const VirtualRegister REG_ST3;
extern const VirtualRegister REG_ST4;
extern const VirtualRegister REG_ST5;
extern const VirtualRegister REG_ST6;
extern const VirtualRegister REG_ST7;
extern const VirtualRegister REG_XCR0;
extern const VirtualRegister REG_XMM0;
extern const VirtualRegister REG_XMM1;
extern const VirtualRegister REG_XMM2;
extern const VirtualRegister REG_XMM3;
extern const VirtualRegister REG_XMM4;
extern const VirtualRegister REG_XMM5;
extern const VirtualRegister REG_XMM6;
extern const VirtualRegister REG_XMM7;
extern const VirtualRegister REG_XMM8;
extern const VirtualRegister REG_XMM9;
extern const VirtualRegister REG_XMM10;
extern const VirtualRegister REG_XMM11;
extern const VirtualRegister REG_XMM12;
extern const VirtualRegister REG_XMM13;
extern const VirtualRegister REG_XMM14;
extern const VirtualRegister REG_XMM15;
extern const VirtualRegister REG_XMM16;
extern const VirtualRegister REG_XMM17;
extern const VirtualRegister REG_XMM18;
extern const VirtualRegister REG_XMM19;
extern const VirtualRegister REG_XMM20;
extern const VirtualRegister REG_XMM21;
extern const VirtualRegister REG_XMM22;
extern const VirtualRegister REG_XMM23;
extern const VirtualRegister REG_XMM24;
extern const VirtualRegister REG_XMM25;
extern const VirtualRegister REG_XMM26;
extern const VirtualRegister REG_XMM27;
extern const VirtualRegister REG_XMM28;
extern const VirtualRegister REG_XMM29;
extern const VirtualRegister REG_XMM30;
extern const VirtualRegister REG_XMM31;
extern const VirtualRegister REG_YMM0;
extern const VirtualRegister REG_YMM1;
extern const VirtualRegister REG_YMM2;
extern const VirtualRegister REG_YMM3;
extern const VirtualRegister REG_YMM4;
extern const VirtualRegister REG_YMM5;
extern const VirtualRegister REG_YMM6;
extern const VirtualRegister REG_YMM7;
extern const VirtualRegister REG_YMM8;
extern const VirtualRegister REG_YMM9;
extern const VirtualRegister REG_YMM10;
extern const VirtualRegister REG_YMM11;
extern const VirtualRegister REG_YMM12;
extern const VirtualRegister REG_YMM13;
extern const VirtualRegister REG_YMM14;
extern const VirtualRegister REG_YMM15;
extern const VirtualRegister REG_YMM16;
extern const VirtualRegister REG_YMM17;
extern const VirtualRegister REG_YMM18;
extern const VirtualRegister REG_YMM19;
extern const VirtualRegister REG_YMM20;
extern const VirtualRegister REG_YMM21;
extern const VirtualRegister REG_YMM22;
extern const VirtualRegister REG_YMM23;
extern const VirtualRegister REG_YMM24;
extern const VirtualRegister REG_YMM25;
extern const VirtualRegister REG_YMM26;
extern const VirtualRegister REG_YMM27;
extern const VirtualRegister REG_YMM28;
extern const VirtualRegister REG_YMM29;
extern const VirtualRegister REG_YMM30;
extern const VirtualRegister REG_YMM31;
extern const VirtualRegister REG_ZMM0;
extern const VirtualRegister REG_ZMM1;
extern const VirtualRegister REG_ZMM2;
extern const VirtualRegister REG_ZMM3;
extern const VirtualRegister REG_ZMM4;
extern const VirtualRegister REG_ZMM5;
extern const VirtualRegister REG_ZMM6;
extern const VirtualRegister REG_ZMM7;
extern const VirtualRegister REG_ZMM8;
extern const VirtualRegister REG_ZMM9;
extern const VirtualRegister REG_ZMM10;
extern const VirtualRegister REG_ZMM11;
extern const VirtualRegister REG_ZMM12;
extern const VirtualRegister REG_ZMM13;
extern const VirtualRegister REG_ZMM14;
extern const VirtualRegister REG_ZMM15;
extern const VirtualRegister REG_ZMM16;
extern const VirtualRegister REG_ZMM17;
extern const VirtualRegister REG_ZMM18;
extern const VirtualRegister REG_ZMM19;
extern const VirtualRegister REG_ZMM20;
extern const VirtualRegister REG_ZMM21;
extern const VirtualRegister REG_ZMM22;
extern const VirtualRegister REG_ZMM23;
extern const VirtualRegister REG_ZMM24;
extern const VirtualRegister REG_ZMM25;
extern const VirtualRegister REG_ZMM26;
extern const VirtualRegister REG_ZMM27;
extern const VirtualRegister REG_ZMM28;
extern const VirtualRegister REG_ZMM29;
extern const VirtualRegister REG_ZMM30;
extern const VirtualRegister REG_ZMM31;

}  // namespace arch
}  // namespace granary

#endif  // ARCH_X86_64_REGISTER_H_
