/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef MODULE
# define MODULE
#endif

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "os/linux/kernel/module.h"

extern struct LinuxKernelModule *granary_kernel_modules;
static struct LinuxKernelModule **last_module_ptr = NULL;

// Initialize a new `KernelModule` from a `struct module`. A `KernelModule`
// is a stripped down `struct module` that contains enough information for
// Granary to create its own `Module` structure from.
static struct LinuxKernelModule *AllocModule(const struct module *mod) {
  struct LinuxKernelModule *kmod = kmalloc(sizeof(struct LinuxKernelModule),
                                           GFP_NOWAIT);
  kmod->name = mod->name;
  kmod->kind = mod == THIS_MODULE ? GRANARY_MODULE : KERNEL_MODULE;
  kmod->module = NULL;
  kmod->core_text_begin = (uintptr_t) mod->module_core;
  kmod->core_text_end = kmod->core_text_begin + mod->core_text_size;
  kmod->init_text_begin = 0;
  kmod->init_text_end = 0;
  kmod->next = NULL;
  if (MODULE_STATE_LIVE != mod->state && MODULE_STATE_GOING != mod->state) {
    kmod->init_text_begin = (uintptr_t) mod->module_init;
    kmod->init_text_begin = kmod->init_text_begin + mod->init_text_size;
  }
  return kmod;
}

// Treat the kernel as one large module.
static struct LinuxKernelModule kernel_module = {
  .name = "kernel",
  .kind = KERNEL,
  .module = NULL,
  .core_text_begin = 0xffffffff80000000ULL,
  .core_text_end = 0xffffffffa0000000ULL,
  .init_text_begin = 0,
  .init_text_end = 0,
  .next = NULL
};


// The kernel's internal module list. Guarded by `modules_lock`.
extern struct list_head *linux_modules;
extern struct mutex *linux_module_mutex;

void InitModules(void) {
  struct module *mod = NULL;
  struct LinuxKernelModule *kmod = NULL;

  granary_kernel_modules = &kernel_module;
  last_module_ptr = &(kernel_module.next);

  mutex_lock(linux_module_mutex);
  list_for_each_entry(mod, linux_modules, list) {
    kmod = AllocModule(mod);
    *last_module_ptr = kmod;
    last_module_ptr = &(kmod->next);
  }
  mutex_unlock(linux_module_mutex);
}
