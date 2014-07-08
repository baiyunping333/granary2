/* Copyright 2014 Peter Goodman, all rights reserved. */

#define GRANARY_INTERNAL

#include "granary/arch/base.h"

#include "granary/base/option.h"
#include "granary/base/string.h"

#include "granary/cfg/basic_block.h"

#include "granary/code/compile.h"
#include "granary/code/edge.h"
#include "granary/code/metadata.h"

#include "granary/cache.h"
#include "granary/context.h"
#include "granary/index.h"

GRANARY_DEFINE_positive_int(edge_cache_slab_size, 16,
    "The number of pages allocated at once to store edge code. Each "
    "environment maintains its own edge code allocator. The default value is "
    "16 pages per slab.");

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
extern void GenerateDirectEdgeEntryCode(ContextInterface *context,
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
extern void GenerateIndirectEdgeEntryCode(ContextInterface *context,
                                          CachePC edge);

// Instantiate an indirect out-edge template. The indirect out-edge will
// compare the target of a CFI with `app_pc`, and if the values match, then
// will jump to `cache_pc`, otherwise a fall-back is taken.
//
// Note: This function has an architecture-specific implementation.
//
// Note: This function is protected by `Context::indirect_edge_list_lock`.
extern void InstantiateIndirectEdge(IndirectEdge *edge, CachePC edge_pc,
                                    AppPC app_pc, CachePC cache_pc);
}  // namespace arch
namespace {

static CachePC CreateDirectEntryCode(ContextInterface *context,
                                     CodeCache *edge_code_cache) {
  auto entry_code = edge_code_cache->AllocateBlock(
      arch::DIRECT_EDGE_CODE_SIZE_BYTES);
  CodeCacheTransaction transaction(
      edge_code_cache, entry_code,
      entry_code + arch::DIRECT_EDGE_CODE_SIZE_BYTES);
  arch::GenerateDirectEdgeEntryCode(context, entry_code);
  return entry_code;
}

static CachePC CreateIndirectEntryCode(ContextInterface *context,
                                       CodeCache *edge_code_cache) {
  auto entry_code = edge_code_cache->AllocateBlock(
      arch::INDIRECT_EDGE_CODE_SIZE_BYTES);
  CodeCacheTransaction transaction(
      edge_code_cache, entry_code,
      entry_code + arch::INDIRECT_EDGE_CODE_SIZE_BYTES);
  arch::GenerateIndirectEdgeEntryCode(context, entry_code);
  return entry_code;
}

// Register internal meta-data.
static void InitMetaData(MetaDataManager *metadata_manager) {
  metadata_manager->Register<ModuleMetaData>();
  metadata_manager->Register<CacheMetaData>();
  metadata_manager->Register<LiveRegisterMetaData>();
  metadata_manager->Register<StackMetaData>();
  metadata_manager->Register<IndexMetaData>();
}

// Tell Granary about all loaded tools.
static void InitTools(ToolManager *tool_manager, const char *tool_names) {
  ForEachCommaSeparatedString<MAX_TOOL_NAME_LEN>(
      tool_names,
      [=] (const char *tool_name) {
        tool_manager->Register(tool_name);
      });

  // Do a dummy allocation and free of all tools. Tools register meta-data
  // through their constructors and so this will get all tool+option-specific
  // meta-data registered.
  tool_manager->FreeTools(tool_manager->AllocateTools());
}

}  // namespace

ContextInterface::~ContextInterface(void) {}

Context::Context(const char *tool_names)
    : module_manager(this),
      metadata_manager(),
      tool_manager(this),
      edge_code_cache(FLAG_edge_cache_slab_size),
      direct_edge_entry_code(CreateDirectEntryCode(this, &edge_code_cache)),
      indirect_edge_entry_code(CreateIndirectEntryCode(this, &edge_code_cache)),
      edge_list_lock(),
      patched_edge_list(nullptr),
      unpatched_edge_list(nullptr),
      indirect_edge_list_lock(),
      indirect_edge_list(nullptr),
      code_cache_index(new Index) {
  InitMetaData(&metadata_manager);

  // Tell this environment about all loaded modules.
  module_manager.RegisterAllBuiltIn();

  InitTools(&tool_manager, tool_names);
}

namespace {

// Free a linked list of edges.
template <typename EdgeT>
static void FreeEdgeList(EdgeT *edge) {
  EdgeT *next_edge = nullptr;
  for (; edge; edge = next_edge) {
    next_edge = edge->next;
    delete edge;
  }
}

static void InitModuleMeta(ModuleManager *module_manager, BlockMetaData *meta,
                           AppPC start_pc) {
  auto module_meta = MetaDataCast<ModuleMetaData *>(meta);
  auto module = module_manager->FindByAppPC(start_pc);
  module_meta->start_pc = start_pc;
  module_meta->source = module->OffsetOf(start_pc);
}

}  // namespace

Context::~Context(void) {
  FreeEdgeList(patched_edge_list);
  FreeEdgeList(unpatched_edge_list);
  FreeEdgeList(indirect_edge_list);
}

// Allocate and initialize some `BlockMetaData`. This will also set-up the
// `ModuleMetaData` within the `BlockMetaData`.
BlockMetaData *Context::AllocateBlockMetaData(AppPC start_pc) {
  auto meta = AllocateEmptyBlockMetaData();
  InitModuleMeta(&module_manager, meta, start_pc);
  return meta;
}

// Allocate and initialize some `BlockMetaData`, based on some existing
// meta-data `meta`.
BlockMetaData *Context::AllocateBlockMetaData(
    const BlockMetaData *meta_template, AppPC start_pc) {
  auto meta = meta_template->Copy();
  InitModuleMeta(&module_manager, meta, start_pc);
  return meta;
}

// Allocate and initialize some empty `BlockMetaData`.
BlockMetaData *Context::AllocateEmptyBlockMetaData(void) {
  return metadata_manager.Allocate();
}

// Register some meta-data with Granary.
void Context::RegisterMetaData(const MetaDataDescription *desc) {
  metadata_manager.Register(const_cast<MetaDataDescription *>(desc));
}

// Allocate instances of the tools that will be used to instrument blocks.
Tool *Context::AllocateTools(void) {
  return tool_manager.AllocateTools();
}

// Free the allocated tools.
void Context::FreeTools(Tool *tools) {
  tool_manager.FreeTools(tools);
}

// Allocate a new code cache.
//
// Note: This should be a lightweight operation as it is usually invoked
//       whilst fine-grained locks are held.
CodeCacheInterface *Context::AllocateCodeCache(void) {
  return new CodeCache();
}

// Flush an entire code cache.
//
// Note: This should be a lightweight operation as it is usually invoked
//       whilst fine-grained locks are held (e.g. schedule for the allocator
//       to be freed).
void Context::FlushCodeCache(CodeCacheInterface *cache) {
  // TODO(pag): Implement me!
  delete cache;  // TODO(pag): This isn't actually right!!!
  GRANARY_UNUSED(cache);
}


// Allocates a direct edge data structure, as well as the code needed to
// back the direct edge.
DirectEdge *Context::AllocateDirectEdge(const BlockMetaData *source_block_meta,
                                        BlockMetaData *dest_block_meta) {
  auto edge_code = edge_code_cache.AllocateBlock(
      arch::DIRECT_EDGE_CODE_SIZE_BYTES);
  auto edge = new DirectEdge(source_block_meta, dest_block_meta, edge_code);

  do {  // Generate a small stub of code specific to this `DirectEdge`.
    CodeCacheTransaction transaction(
        &edge_code_cache, edge_code,
        edge_code + arch::DIRECT_EDGE_CODE_SIZE_BYTES);
    arch::GenerateDirectEdgeCode(edge, direct_edge_entry_code);
  } while (0);

  do {  // Add the edge to the unpatched list.
    FineGrainedLocked locker(&edge_list_lock);
    edge->next = unpatched_edge_list;
    unpatched_edge_list = edge;
  } while (0);

  return edge;
}

// Allocates an indirect edge data structure.
IndirectEdge *Context::AllocateIndirectEdge(
    const BlockMetaData *source_block_meta,
    const BlockMetaData *dest_block_meta) {
  auto edge = new IndirectEdge(source_block_meta, dest_block_meta,
                               indirect_edge_entry_code);
  FineGrainedLocked locker(&indirect_edge_list_lock);
  edge->next = indirect_edge_list;
  indirect_edge_list = edge;
  return edge;
}

// Instantiates an indirect edge. This creates an out-edge that targets
// `cache_pc` if the indirect CFI being taken is trying to jump to `app_pc`.
// `edge->out_edge_pc` is updated in place to reflect the new target.
void Context::InstantiateIndirectEdge(IndirectEdge *edge, AppPC app_pc,
                                      CachePC cache_pc) {
  auto alloc_amount = static_cast<int>(
      edge->end_out_edge_template - edge->begin_out_edge_template +
      arch::INDIRECT_OUT_EDGE_CODE_PADDING_BYTES);
  alloc_amount += GRANARY_ALIGN_FACTOR(alloc_amount, 16);
  auto edge_pc = edge_code_cache.AllocateBlock(alloc_amount);
  FineGrainedLocked locker(&indirect_edge_list_lock);
  CodeCacheTransaction transaction(&edge_code_cache,
                                   edge_pc, edge_pc + alloc_amount);
  arch::InstantiateIndirectEdge(edge, edge_pc, app_pc, cache_pc);
}

// Get a pointer to this context's code cache index.
LockedIndex *Context::CodeCacheIndex(void) {
  return &code_cache_index;
}

}  // namespace granary
