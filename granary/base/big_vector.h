/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_BASE_BIG_VECTOR_H_
#define GRANARY_BASE_BIG_VECTOR_H_

#include "granary/base/base.h"
#include "granary/base/type_trait.h"

namespace granary {
namespace detail {

GRANARY_INTERNAL_DEFINITION class BigVectorSlab;

// Generic big vector implementation.
class BigVectorImpl {
 protected:
  // Initialize the big vector.
  BigVectorImpl(size_t align_, size_t size_);

  // Free all backing slabs.
  ~BigVectorImpl(void);

  // Find a pointer to the first element in a slab that contains the element
  // at index `index`.
  void *FindObjectPointer(size_t index);

 private:
  BigVectorImpl(void) = delete;

  // Allocate a new slab.
  void AllocateSlab(void);

  GRANARY_POINTER(detail::BigVectorSlab) *slabs;
  GRANARY_POINTER(detail::BigVectorSlab *) *next_slab;

  // Alignment and size of objects contained in this big vector.
  const size_t align;
  const size_t size;

  // Number of objects per slab.
  size_t num_objs_per_slab;

  GRANARY_DISALLOW_COPY_AND_ASSIGN(BigVectorImpl);
};

}  // namespace detail

// Represents a scalable array that is expected to have many entries, and so
// will be backed by a lot of memory.
//
// Note: This vector guarantees that elements in the vector will not be moved.
//       Therefore, a pointer to an element in the vector will remain the
//       same across vector resize operations.
//
// Note: This is only for plain-old-data types or data types with trivial
//       constructors and data-structures. The assumption is that zero-
//       initialization is sufficient to initialize an instance of the type `T`.
template <typename T>
class BigVector : protected detail::BigVectorImpl {
 public:
  inline BigVector(void)
      : detail::BigVectorImpl(alignof(T), sizeof(T)) {}

  // Access the element at index `index`.
  template <typename I, typename EnableIf<IsInteger<I>::RESULT>::Type=0>
  inline T &operator[](I index) {
    return *reinterpret_cast<T *>(
        FindObjectPointer(static_cast<size_t>(index)));
  }

  // TODO(oag): Add `Size` method.
  // TODO(pag): Add `Append` method.
  // TODO(pag): Make iterable.

 private:
  GRANARY_DISALLOW_COPY_AND_ASSIGN_TEMPLATE(BigVector, (T));
};

}  // namespace granary

#endif  // GRANARY_BASE_BIG_VECTOR_H_
