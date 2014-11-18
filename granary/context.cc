/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "arch/base.h"
#include "arch/context.h"

#include "granary/base/container.h"
#include "granary/base/option.h"
#include "granary/base/string.h"

#include "granary/cfg/basic_block.h"

#include "granary/code/compile.h"
#include "granary/code/edge.h"

#include "granary/breakpoint.h"
#include "granary/cache.h"
#include "granary/context.h"
#include "granary/index.h"
#include "granary/metadata.h"

#include "os/module.h"

GRANARY_DEFINE_positive_int(block_cache_slab_size, 512,
    "The number of pages allocated at once to store basic block code. Each "
    "context maintains its own block code allocator. The default value is "
    "`512` pages per slab (2MiB).");

GRANARY_DEFINE_positive_int(edge_cache_slab_size, 256,
    "The number of pages allocated at once to store edge code. Each "
    "context maintains its own edge code allocator. The default value is "
    "`256` pages per slab (1MiB).");

GRANARY_DECLARE_string(tools);

namespace granary {
namespace arch {

// Generates the direct edge entry code for getting onto a Granary private
// stack, disabling interrupts, etc.
//
// This code takes a pointer to the context so that the code generated will
// be able to pass the context pointer directly to `granary::EnterGranary`.
// This allows us to avoid saving the context pointer in the `DirectEdge`.
//
// Note: This has an architecture-specific implementation.
extern void GenerateDirectEdgeEntryCode(Context *context,
                                        CachePC edge);

// Generates the direct edge code for a given `DirectEdge` structure.
//
// Note: This has an architecture-specific implementation.
extern void GenerateDirectEdgeCode(DirectEdge *edge, CachePC edge_entry_code);

// Generates the indirect edge entry code for getting onto a Granary private
// stack, disabling interrupts, etc.
//
// This code takes a pointer to the context so that the code generated will
// be able to pass the context pointer directly to `granary::EnterGranary`.
// This allows us to avoid saving the context pointer in the `IndirectEdge`.
//
// Note: This has an architecture-specific implementation.
extern void GenerateIndirectEdgeEntryCode(Context *context,
                                          CachePC edge);

// Generates code that disables interrupts.
//
// Note: This has an architecture-specific implementation.
extern void GenerateInterruptDisableCode(Context *, CachePC pc);

// Generates code that re-enables interrupts (if they were disabled by the
// interrupt disabling routine).
//
// Note: This has an architecture-specific implementation.
extern void GenerateInterruptEnableCode(Context *, CachePC pc);

// Generates the wrapper code for a context callback.
//
// Note: This has an architecture-specific implementation.
extern Callback *GenerateContextCallback(Context *, CodeCache *cache,
                                         AppPC func_pc);

// Generates the wrapper code for an outline callback.
//
// Note: This has an architecture-specific implementation.
extern Callback *GenerateInlineCallback(CodeCache *cache,
                                         InlineFunctionCall *call);

}  // namespace arch
namespace {

template <typename T>
static CachePC GenerateCode(Context *context, CodeCache *cache,
                            T generator, size_t size) {
  auto code = cache->AllocateBlock(size);
  CodeCacheTransaction transaction(cache, code, code + size);
  generator(context, code);
  return code;
}

// Free a linked list of edges.
template <typename EdgeT>
static void FreeEdgeList(EdgeT *edge) {
  EdgeT *next_edge = nullptr;
  for (; edge; edge = next_edge) {
    next_edge = edge->next;
    delete edge;
  }
}

template <typename T>
static void FreeCallbacks(T &callback_map) {
  for (auto cb : callback_map.Values()) {
    delete cb;
  }
}

}  // namespace

Context::Context(void)
    : tool_manager(this),
      block_code_cache(static_cast<size_t>(FLAG_block_cache_slab_size),
                       kBlockCodeCache),
      edge_code_cache(static_cast<size_t>(FLAG_edge_cache_slab_size),
                      kEdgeCodeCache),
      direct_edge_entry_code(
          GenerateCode(this, &edge_code_cache,
                       arch::GenerateDirectEdgeEntryCode,
                       arch::DIRECT_EDGE_ENTRY_CODE_SIZE_BYTES)),
      indirect_edge_entry_code(
          GenerateCode(this, &edge_code_cache,
                       arch::GenerateIndirectEdgeEntryCode,
                       arch::INDIRECT_EDGE_ENTRY_CODE_SIZE_BYTES)),
      disable_interrupts_code(
          GenerateCode(this, &edge_code_cache,
                       arch::GenerateInterruptDisableCode,
                       arch::DIRECT_EDGE_ENTRY_CODE_SIZE_BYTES)),
      enable_interrupts_code(
          GenerateCode(this, &edge_code_cache,
                       arch::GenerateInterruptEnableCode,
                       arch::DIRECT_EDGE_ENTRY_CODE_SIZE_BYTES)),
      edge_list_lock(),
      patched_edge_list(nullptr),
      unpatched_edge_list(nullptr),
      indirect_edge_list_lock(),
      indirect_edge_list(nullptr),
      code_cache_index(new Index),
      context_callbacks_lock(),
      context_callbacks(),
      arg_callbacks_lock(),
      outline_callbacks() {}

Context::~Context(void) {
  FreeEdgeList(patched_edge_list);
  FreeEdgeList(unpatched_edge_list);
  FreeEdgeList(indirect_edge_list);
  FreeCallbacks(context_callbacks);
  FreeCallbacks(outline_callbacks);
}

// Initialize all tools from a comma-separated list of tools.
void Context::InitTools(InitReason reason) {

  // Force register some tools that should get priority over all others.
  tool_manager.Add(GRANARY_IF_KERNEL_ELSE("kernel", "user"));

  // Registered early so that all returns start off specialized by default.
  tool_manager.Add("transparent_returns_early");

#ifdef GRANARY_WITH_VALGRIND
  // Auto-registered so that `aligned_alloc` and `free` are always wrapped to
  // execute natively (and so are ideally instrumented by Valgrind to help
  // catch memory access bugs).
  tool_manager.Add("valgrind");
#endif  // GRANARY_WITH_VALGRIND

  if (FLAG_tools) {
    // Register tools specified at the command-line.
    ForEachCommaSeparatedString<MAX_TOOL_NAME_LEN>(
        FLAG_tools,
        [&] (const char *tool_name) {
          tool_manager.Add(tool_name);
        });
  }

  // Registered last so that transparent returns applies to all control-flow
  // after every other tool has made control-flow decisions.
  tool_manager.Add("transparent_returns_late");

  // Initialize all tools. Tool initialization is typically where tools will
  // register their specific their block meta-data, therefore it is important
  // to initialize all tools before finalizing the meta-data manager.
  auto tools = tool_manager.AllocateTools();
  for (auto tool : ToolIterator(tools)) {
    tool->Init(reason);
  }
  tool_manager.FreeTools(tools);
}

// Exit all tools. Tool `Exit` methods should restore any global state to
// their initial values.
void Context::ExitTools(ExitReason reason) {
  auto tools = tool_manager.AllocateTools();
  for (auto tool : ToolIterator(tools)) {
    tool->Exit(reason);
  }
  tool_manager.FreeTools(tools);
}

// Allocate and initialize some `BlockMetaData`. This will also set-up the
// `AppMetaData` within the `BlockMetaData`.
BlockMetaData *Context::AllocateBlockMetaData(AppPC start_pc) {
  auto meta = new BlockMetaData;
  MetaDataCast<AppMetaData *>(meta)->start_pc = start_pc;
  return meta;
}

// Allocate and initialize some `BlockMetaData`, based on some existing
// meta-data `meta`.
BlockMetaData *Context::AllocateBlockMetaData(
    const BlockMetaData *meta_template, AppPC start_pc) {
  auto meta = meta_template->Copy();
  MetaDataCast<AppMetaData *>(meta)->start_pc = start_pc;
  return meta;
}

// Allocate instances of the tools that will be used to instrument blocks.
InstrumentationTool *Context::AllocateTools(void) {
  return tool_manager.AllocateTools();
}

// Free the allocated tools.
void Context::FreeTools(InstrumentationTool *tools) {
  tool_manager.FreeTools(tools);
}

// Allocates a direct edge data structure, as well as the code needed to
// back the direct edge.
DirectEdge *Context::AllocateDirectEdge(BlockMetaData *dest_block_meta) {
  auto edge_code = edge_code_cache.AllocateBlock(
      arch::DIRECT_EDGE_CODE_SIZE_BYTES);
  auto edge = new DirectEdge(dest_block_meta, edge_code);

  do {  // Generate a small stub of code specific to this `DirectEdge`.
    CodeCacheTransaction transaction(
        &edge_code_cache, edge_code,
        edge_code + arch::DIRECT_EDGE_CODE_SIZE_BYTES);
    arch::GenerateDirectEdgeCode(edge, direct_edge_entry_code);
  } while (0);

  do {  // Add the edge to the unpatched list.
    SpinLockedRegion locker(&edge_list_lock);
    edge->next = unpatched_edge_list;
    unpatched_edge_list = edge;
  } while (0);

  return edge;
}

// Allocates an indirect edge data structure.
IndirectEdge *Context::AllocateIndirectEdge(
    const BlockMetaData *dest_block_meta) {
  auto edge = new IndirectEdge(dest_block_meta, indirect_edge_entry_code);
  SpinLockedRegion locker(&indirect_edge_list_lock);
  edge->next = indirect_edge_list;
  indirect_edge_list = edge;
  return edge;
}

// Returns a pointer to the code cache that is used for allocating code for
// basic blocks.
CodeCache *Context::BlockCodeCache(void) {
  return &block_code_cache;
}

// Get a pointer to this context's code cache index.
LockedIndex *Context::CodeCacheIndex(void) {
  return &code_cache_index;
}

// Invalidate blocks that have been committed to the code cache index. This
// invalidates all blocks in the range `[begin_addr, end_addr)`.
void Context::InvalidateIndexedBlocks(AppPC begin_addr, AppPC end_addr) {
  BlockMetaData *meta(nullptr);
  do {
    LockedIndexTransaction transaction(&code_cache_index);
    meta = transaction.RemoveRange(begin_addr, end_addr);
  } while (0);

  // TODO(pag): Do something with `meta`!! This is a major memory leak at the
  //            moment.
}

// Returns a pointer to the `CachePC` associated with the context-callable
// function at `func_addr`.
const arch::Callback *Context::ContextCallback(AppPC func_pc) {
  SpinLockedRegion locker(&arg_callbacks_lock);
  auto *&cb(context_callbacks[func_pc]);
  if (!cb) {
    cb = arch::GenerateContextCallback(this, &edge_code_cache, func_pc);
  }
  return cb;
}

// Returns a pointer to the code cache code associated with some outline-
// callable function at `func_addr`.
const arch::Callback *Context::InlineCallback(InlineFunctionCall *call) {
  SpinLockedRegion locker(&arg_callbacks_lock);
  auto &cb(outline_callbacks[call->target_app_pc]);
  if (!cb) {
    cb = arch::GenerateInlineCallback(&edge_code_cache, call);
  }
  return cb;
}

#ifdef GRANARY_WHERE_kernel
// Returns a pointer to the code that can disable interrupts.
CachePC Context::DisableInterruptCode(void) const {
  return disable_interrupts_code;
}

// Returns a pointer to the code that can enable interrupts.
CachePC Context::EnableInterruptCode(void) const {
  return enable_interrupts_code;
}
#endif  // GRANARY_WHERE_kernel

namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
GRANARY_EARLY_GLOBAL static Container<Context> gContext;
#pragma clang diagnostic pop
}  // namespace

// Initializes a new active context.
void InitContext(InitReason reason) {
  gContext.Construct();
  gContext->InitTools(reason);
}

// Destroys the active context.
void ExitContext(ExitReason reason) {
  gContext->ExitTools(reason);
  gContext.Destroy();
}

// Loads the active context.
Context *GlobalContext(void) {
  return gContext.AddressOf();
}

}  // namespace granary
