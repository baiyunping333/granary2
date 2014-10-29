/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef CLIENTS_UTIL_TYPES_H_
#define CLIENTS_UTIL_TYPES_H_

extern "C" {

#ifdef _GNU_SOURCE
# undef _GNU_SOURCE
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#pragma clang diagnostic ignored "-Wextern-c-compat"
#pragma clang diagnostic ignored "-Wextra-semi"

#ifdef GRANARY_OS_linux
# ifdef GRANARY_WHERE_kernel

typedef bool K_Bool;

#   include "generated/linux_kernel/types.h"

# else
#   include "generated/linux_user/types.h"
# endif
#else
# error "Unrecognized operating system."
#endif

#pragma clang diagnostic pop

}  // extern C

#endif  // CLIENTS_UTIL_TYPES_H_
