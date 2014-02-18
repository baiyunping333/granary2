/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_DRIVER_XED2_INTEL64_XED_H_
#define GRANARY_DRIVER_XED2_INTEL64_XED_H_

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
extern "C" {
#define XED_DLL
#include "dependencies/xed2-intel64/include/xed-interface.h"
}  // extern C
#pragma clang diagnostic pop

#endif  // GRANARY_DRIVER_XED2_INTEL64_XED_H_
