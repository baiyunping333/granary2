/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/base/base.h"
#include "granary/base/lock.h"
#include "granary/base/string.h"

#include "os/logging.h"
#include "os/lock.h"

// Visible from GDB.
extern "C" {
char granary_log_buffer[32768 << 5] = {'\0'};
unsigned long granary_log_buffer_index = 0;
}  // extern C

namespace granary {
namespace os {

// Initialize the logging mechanism.
void InitLog(void) {}

// Exit the log.
void ExitLog(void) {}

namespace {
static os::Lock log_buffer_lock;
}

// Log something.
//
// TODO(pag): This is totally unsafe! It can easily overflow.
size_t Log(LogLevel, const char *format, ...) {
  os::LockedRegion locker(&log_buffer_lock);
  va_list args;
  va_start(args, format);
  auto ret = 0UL;
  if (granary_log_buffer_index < sizeof granary_log_buffer) {
    ret = VFormat(&(granary_log_buffer[granary_log_buffer_index]),
                  sizeof granary_log_buffer - granary_log_buffer_index - 1,
                  format, args);
    granary_log_buffer_index += ret;
  }
  va_end(args);
  return ret;
}

}  // namespace os
}  // namespace granary
