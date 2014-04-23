/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_CODE_ASSEMBLE_9_LOG_FRAGMENTS_H_
#define GRANARY_CODE_ASSEMBLE_9_LOG_FRAGMENTS_H_

#ifndef GRANARY_INTERNAL
# error "This code is internal to Granary."
#endif

namespace granary {

// Forward declarations.
enum LogLevel : int;

// Log a list of fragments as a DOT digraph.
void Log(LogLevel level, FragmentList *frags);

}  // namespace granary

#endif  // GRANARY_CODE_ASSEMBLE_9_LOG_FRAGMENTS_H_
