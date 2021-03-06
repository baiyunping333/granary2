/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef GRANARY_METADATA_H_
#define GRANARY_METADATA_H_

#include "granary/base/base.h"
#include "granary/base/container.h"
#include "granary/base/new.h"
#include "granary/base/operator.h"
#include "granary/base/type_trait.h"
#include "granary/base/pc.h"

// Used to explicitly instantiate this so that it is available to shared
// libraries.
#ifdef GRANARY_EXTERNAL
# define GRANARY_SHARE_METADATA(meta_class)
#else
# define GRANARY_SHARE_METADATA(meta_class)
#endif

namespace granary {

// Forward declarations.
class BlockMetaData;

// All types of meta-data.
template <typename T>
class ToolMetaData {
 public:

  // Join some meta-data associated with an existing basic block (`existing`)
  // with the meta-data template associated with some indirect basic block
  // (`this`). The default behavior here to to inherit all information from
  // the existing block's meta-data.
  void Join(const T &existing) {
    CopyConstruct<T>(this, &existing);
  }
};

// Serializable meta-data (i.e. immutable once committed to the code cache)
// must implement the `Hash` and `Equals` methods, and extend this template
// using CRTP.
template <typename T>
class IndexableMetaData : public ToolMetaData<T> {
 public:
  bool Equals(const T &that) const;
};

// Mutable meta-data (i.e. mutable even after committed to the code cache)
// must extend this base class.
template <typename T>
class MutableMetaData : public ToolMetaData<T> {};

// Used to decide whether two pieces of unifiable meta-data can unify.
//
// Note: The particular values are significant, as they allow us to do a `MAX`
//       operation to find what the status is of many "sub meta-datas".
enum UnificationStatus {
  kUnificationStatusAccept  = 0, // Unifies perfectly.
  kUnificationStatusAdapt   = 1, // Doesn't unify perfectly, but can be adapted.
  kUnificationStatusReject  = 2 // Cannot be unified / adapted.
};

// Unifiable meta-data, i.e. meta-data that behaves a bit like indexable meta-
// data, but doesn't directly participate in the indexing process. The idea here
// is that sometimes we want to generate new versions of basic blocks, and other
// times we want to be able to re-use old versions, but the old versions aren't
// necessarily perfectly suited, so we need to adapt to them.
template <typename T>
class UnifiableMetaData : public ToolMetaData<T> {
 public:
  UnificationStatus CanUnifyWith(const T &that) const;
};

// Describes whether some type is an indexable meta-data type.
template <typename T>
struct IsIndexableMetaData {
  enum {
    RESULT = !!std::is_convertible<T *, IndexableMetaData<T> *>::value
  };
};

// Describes whether some type is an indexable meta-data type.
template <typename T>
struct IsMutableMetaData {
  enum {
    RESULT = !!std::is_convertible<T *, MutableMetaData<T> *>::value
  };
};

// Describes whether some type is an indexable meta-data type.
template <typename T>
struct IsUnifiableMetaData {
  enum {
    RESULT = !!std::is_convertible<T *, UnifiableMetaData<T> *>::value
  };
};

// Describes whether some type is a meta-data type.
template <typename T>
struct IsMetaData {
  enum {
    RESULT = IsIndexableMetaData<T>::RESULT ||
             IsMutableMetaData<T>::RESULT ||
             IsUnifiableMetaData<T>::RESULT
  };
};

template <typename T, bool kIsIndexable, bool kIsMutable, bool kIsUnifiable>
struct MetaDataDescriptor;

// Describes some generic meta-data in a way that Granary understands.
class MetaDataDescription {
 public:
  // Globally unique ID for this meta-data description. Granary internally
  // uses this ID to operate with the same meta-data, but registered within
  // different environments.
  GRANARY_CONST int id;

  // Offset of T within a `BlockMetaData`.
  uintptr_t offset;

  // Where in the generic meta-data is this specific meta-data.
  const size_t size;
  const size_t align;

  // Virtual table of operations on the different classes of meta-data.
  void (* const initialize)(void *);
  void (* const copy_initialize)(void *, const void *);
  void (* const destroy)(void *);
  bool (* const compare_equals)(const void *, const void *);
  UnificationStatus (* const can_unify)(const void *, const void *);
  void (* const join)(void *, const void *);
};

// Used to get meta-data descriptions, as well as a mechanism of ensuring that
// specific granary-interal versions of meta-data are exported to clients.
template <typename T>
class GetMetaDataDescription {
 public:
  static_assert(IsMetaData<T>::RESULT, "Type `T` must be a meta-data type.");

  inline static MetaDataDescription *Get(void) {
    return &(MetaDataDescriptor<
      T,
      IsIndexableMetaData<T>::RESULT,
      IsMutableMetaData<T>::RESULT,
      IsUnifiableMetaData<T>::RESULT
    >::kDescription);
  }
 private:
  GetMetaDataDescription(void) = delete;
};

// Descriptor for some indexable meta-data.
template <typename T>
struct MetaDataDescriptor<T, true, false, false> {
 public:
  static MetaDataDescription kDescription GRANARY_EARLY_GLOBAL;
};

// Descriptor for some mutable meta-data.
template <typename T>
struct MetaDataDescriptor<T, false, true, false> {
 public:
  static MetaDataDescription kDescription GRANARY_EARLY_GLOBAL;
};

// Descriptor for some unifiable meta-data.
template <typename T>
struct MetaDataDescriptor<T, false, false, true> {
 public:
  static MetaDataDescription kDescription GRANARY_EARLY_GLOBAL;
};

namespace detail {

// Compare some meta-data for equality.
template <typename T>
bool CompareEquals(const void *a, const void *b) {
  return reinterpret_cast<const T *>(a)->Equals(*reinterpret_cast<const T *>(b));
}

// Join / combine two an existing meta-data `b` into a requested meta-data
// template `a`.
template <typename T>
void Join(void *a, const void *b) {
  return reinterpret_cast<T *>(a)->Join(*reinterpret_cast<const T *>(b));
}

// Compare some meta-data for equality.
template <typename T>
UnificationStatus CanUnify(const void *a, const void *b) {
  return reinterpret_cast<const T *>(a)->CanUnifyWith(
      *reinterpret_cast<const T *>(b));
}
}  // namespace detail

// Indexable.
template <typename T>
MetaDataDescription MetaDataDescriptor<T, true, false, false>::kDescription = {
    -1,
    std::numeric_limits<uintptr_t>::max(),
    sizeof(T),
    alignof(T),
    &(Construct<T>),
    &(CopyConstruct<T>),
    &(Destruct<T>),
    &(detail::CompareEquals<T>),
    nullptr,
    &(detail::Join<T>)
};

// Mutable.
template <typename T>
MetaDataDescription MetaDataDescriptor<T, false, true, false>::kDescription = {
    -1,
    std::numeric_limits<uintptr_t>::max(),
    sizeof(T),
    alignof(T),
    &(Construct<T>),
    &(CopyConstruct<T>),
    &(Destruct<T>),
    nullptr,
    nullptr,
    &(detail::Join<T>)
};

// Unifyable.
template <typename T>
MetaDataDescription MetaDataDescriptor<T, false, false, true>::kDescription = {
    -1,
    std::numeric_limits<uintptr_t>::max(),
    sizeof(T),
    alignof(T),
    &(Construct<T>),
    &(CopyConstruct<T>),
    &(Destruct<T>),
    nullptr,
    &(detail::CanUnify<T>),
    &(detail::Join<T>)
};

// Meta-data about a basic block.
class BlockMetaData {
 public:
  // Initialize a new meta-data instance. This involves separately initializing
  // the contained meta-data within this generic meta-data.
  GRANARY_INTERNAL_DEFINITION BlockMetaData(void);

  // Initialize a new meta-data instance. This initializes the `AppMetaData`
  // as well.
  GRANARY_INTERNAL_DEFINITION explicit BlockMetaData(AppPC app_pc);

  // Destroy a meta-data instance. This involves separately destroying the
  // contained meta-data within this generic meta-data.
  ~BlockMetaData(void) GRANARY_EXTERNAL_DELETE;

  // Create a copy of some meta-data and return a new instance of the copied
  // meta-data.
  GRANARY_INTERNAL_DEFINITION BlockMetaData *Copy(void) const;

  // Compare the serializable components of two generic meta-data instances for
  // strict equality.
  GRANARY_INTERNAL_DEFINITION bool Equals(const BlockMetaData *meta) const;

  // Check to see if this meta-data can unify with some other generic meta-data.
  GRANARY_INTERNAL_DEFINITION
  UnificationStatus CanUnifyWith(const BlockMetaData *meta) const;

  // Combine this meta-data with some other meta-data.
  GRANARY_INTERNAL_DEFINITION
  void JoinWith(const BlockMetaData *meta);

  // Allocate and free this block meta-data.
  GRANARY_INTERNAL_DEFINITION static void *operator new(size_t);
  GRANARY_INTERNAL_DEFINITION static void operator delete(void *address);

 private:
  GRANARY_IF_EXTERNAL( BlockMetaData(void) = delete; )

  GRANARY_DISALLOW_COPY_AND_ASSIGN(BlockMetaData);
};

// Cast some generic meta-data into some specific meta-data.
template <typename T>
inline static T MetaDataCast(BlockMetaData *meta) {
  typedef typename RemoveConst<typename RemovePointer<T>::Type>::Type M;
  return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(meta) +
                             GetMetaDataDescription<M>::Get()->offset);
}

// Cast some generic meta-data into some specific meta-data.
template <typename T>
inline static
const typename RemoveConst<typename RemovePointer<T>::Type>::Type *
MetaDataCast(const BlockMetaData *meta) {
  typedef typename RemoveConst<typename RemovePointer<T>::Type>::Type M;
  return reinterpret_cast<const M *>(reinterpret_cast<uintptr_t>(meta) +
                                     GetMetaDataDescription<M>::Get()->offset);
}


// Initialize the global meta-data manager.
GRANARY_INTERNAL_DEFINITION
void InitMetaData(void);

// Destroy the global meta-data manager.
GRANARY_INTERNAL_DEFINITION
void ExitMetaData(void);

// Register some meta-data with Granary that will be used with this tool.
// This is a convenience method around the `AddMetaData` method that
// operates directly on a meta-data description.
template <typename T>
inline static void AddMetaData(void) {
  AddMetaData(GetMetaDataDescription<T>::Get());
}

// Register some meta-data with the meta-data manager.
void AddMetaData(MetaDataDescription *desc);

// Adds this meta-data to a trace log of recently translated meta-data blocks.
GRANARY_INTERNAL_DEFINITION
void TraceMetaData(const BlockMetaData *meta);

// Useful for linked lists of meta-data.
template <typename T>
class MetaDataLinkedListIterator {
 public:
  typedef MetaDataLinkedListIterator<T> Iterator;

  typedef decltype(reinterpret_cast<T *>(0UL)->next) NextPointerType;
  typedef typename RemovePointer<NextPointerType>::Type NextType;

  typedef typename EnableIf<
      IsConst<NextType>::RESULT,
      const BlockMetaData,
      BlockMetaData>::Type M;

  MetaDataLinkedListIterator(void)
      : curr(nullptr) {}

  MetaDataLinkedListIterator(const Iterator &that)  // NOLINT
      : curr(that.curr) {}

  MetaDataLinkedListIterator(const Iterator &&that)  // NOLINT
      : curr(that.curr) {}

  explicit MetaDataLinkedListIterator(M *first)
      : curr(first) {}

  inline Iterator begin(void) const {
    return *this;
  }

  inline Iterator end(void) const {
    return Iterator();
  }

  inline void operator++(void) {
    curr = MetaDataCast<const T *>(curr)->next;
  }

  inline bool operator!=(const Iterator &that) const {
    return curr != that.curr;
  }

  inline M *operator*(void) const {
    return curr;
  }

  // Returns the last valid element from an iterator.
  static M *Last(Iterator elems) {
    M *last(nullptr);
    for (auto elem : elems) {
      last = elem;
    }
    return last;
  }

  // Returns the last valid element from an iterator.
  static inline M *Last(M *elems_ptr) {
    return Last(Iterator(elems_ptr));
  }

 private:
  M *curr;
};

}  // namespace granary

#endif  // GRANARY_METADATA_H_
