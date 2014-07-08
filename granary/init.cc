/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/arch/init.h"

#include "granary/base/container.h"
#include "granary/base/option.h"
#include "granary/base/string.h"

#include "granary/client.h"
#include "granary/context.h"
#include "granary/init.h"
#include "granary/logging.h"
#include "granary/translate.h"

// TODO(pag): Remove me.
#include "granary/base/cast.h"
#include "granary/base/pc.h"

#ifndef GRANARY_STANDALONE
GRANARY_DEFINE_string(attach_to, "*",
    "Comma-separated list of modules to which granary should attach. Default "
    "is `*`, representing that Granary will attach to all (non-Granary, non-"
    "tool) modules. More specific requests can be made, for example:\n"
    "\t--attach_to=[*,-libc]\t\tAttach to everything but `libc`.\n"
    "\t--attach_to=libc\t\tOnly attach to `libc`.");
#endif  // GRANARY_STANDALONE

extern "C" {
extern void granary_test_mangle(void);
}

GRANARY_DEFINE_string(tools, "",
    "Comma-seprated list of tools to dynamically load on start-up. "
    "For example: `--clients=print_bbs,follow_jumps`.");

GRANARY_DEFINE_bool(help, false,
    "Print this message.");

static uint64_t time_now(void) {
  uint64_t low_order, high_order;
  asm volatile("rdtsc" : "=a" (low_order), "=d" (high_order));
  return (high_order << 32U) | low_order;
}

enum {
  NUM_ITERATIONS = 10,
  MAX_FIB = 20
};

namespace granary {
namespace {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
GRANARY_EARLY_GLOBAL static Container<Context> context;
#pragma clang diagnostic pop

extern "C" int fibonacci(int n);
extern "C" int (*fib_indirect)(int);

extern "C" int (*fib_indirect)(int) = fibonacci;
extern "C" int fibonacci(int n) {
  if (!n) return n;
  if (1 == n) return 1;
  return fib_indirect(n - 1) + fib_indirect(n - 2);
}

}  // namespace

// Initialize Granary.
void Init(const char *granary_path) {

  // Initialize the driver (e.g. XED, DynamoRIO). This usually performs from
  // architecture-specific checks to determine which architectural features
  // are enabled.
  arch::Init();

  // Dynamically load in zero or more clients. In user space, clients are
  // specified on the command-line. In kernel-space, clients are compiled in
  // with the Granary binary.
  //
  // We do this before finding and registering all built-in modules so that
  // module registration picks up on existing clients.
  LoadClients(granary_path);

  if (!FLAG_help) {
    context.Construct(FLAG_tools);

    if (true) {
      auto start_pc = Translate(context.AddressOf(), fibonacci);
      auto fib = UnsafeCast<int (*)(int)>(start_pc);
      for (auto i = 0; i < 30; ++i) {
        Log(LogOutput, "fibonacci(%d) = %d; fib(%d) = %d\n", i,
            fibonacci(i), i, fib(i));
      }

      auto native_start = time_now();
      for (auto iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (auto n = 0; n < MAX_FIB; ++n) {
          fib_indirect(n);
        }
      }
      auto native_end = time_now();

      auto inst_start = time_now();
      for (auto iter = 0; iter < NUM_ITERATIONS; ++iter) {
        for (auto n = 0; n < MAX_FIB; ++n) {
          fib(n);
        }
      }
      auto inst_end = time_now();


      auto ticks_per_native = (native_end - native_start) / NUM_ITERATIONS;
      auto ticks_per_inst = (inst_end - inst_start) / NUM_ITERATIONS;
      Log(LogOutput, "Ticks per native = %lu\nTicks per instrumented = %lu\n",
          ticks_per_native, ticks_per_inst);

    } else {
      //Translate(context.AddressOf(), granary_test_mangle);
    }

    context.Destroy();
  } else {
    PrintAllOptions();
  }

  UnloadClients();
}

}  // namespace granary
