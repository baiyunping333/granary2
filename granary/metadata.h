/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef GRANARY_METADATA_H_
#define GRANARY_METADATA_H_
#ifdef GRANARY_INTERNAL

#include "granary/base/base.h"
#include "granary/base/new.h"

namespace granary {

// Forward declarations.
class HashFunction;

namespace detail {
class MetaDataDescription;
}  // namespace detail

// TODO(pag): Need a structure that can describe the contents of the meta-data.
// TODO(pag): Need a dummy structure that represents some client meta-data.

enum BasicBlockFlags : uint32_t {
  // Has this meta-data been committed to longer-term storage?
  IS_INTERNED           = (1 << 0),

  // If this basic block has a return from procedure instruction in it, then
  // should that be translated using the identity translation?
  ENABLE_DIRECT_RETURN  = (1 << 1),

  // Should this basic block be run natively? I.e. should be just run the
  // app code instead of instrumenting it?
  RUN_NATIVELY          = (1 << 2)
};

// Meta-data about a basic block. This structure contains a small amount of
// information that is useful to Granary's internal operation, and acts as
// a header to an unknown amount of client/tool-specific meta-data.
class BasicBlockMetaData {
 public:

  // TODO(pag): Should probably pass in some form of allocator or something
  //            that knows how to do a deep copy of the meta-data.
  BasicBlockMetaData *Copy(void) const;

  void Hash(HashFunction *hasher) const;

  const GRANARY_POINTER(detail::MetaDataDescription) * const description;

 private:
  // Tracks internal flags, including whether or not this meta-data has been
  // interned, whether or not (when executing the function containing this
  // block), we would expect the return address to be transparent or non-
  // transparent, and whether or not an annotation was added to this basic block
  // at decode time.
  GRANARY_UINT32(BasicBlockFlags) flags;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(BasicBlockMetaData);
};

}  // namespace granary

#endif  // GRANARY_INTERNAL
#endif  // GRANARY_METADATA_H_