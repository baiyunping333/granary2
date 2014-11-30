/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "arch/cpu.h"
#include "granary/base/lock.h"

namespace granary {

// Acquires the lock, knowing that the lock is currently contended.
void SpinLock::ContendedAcquire(void) {
  do {
    arch::Relax();
  } while (is_locked.load(std::memory_order_relaxed) || !TryAcquire());
}

// Returns true if the lock was acquired.
bool SpinLock::TryAcquire(void) {
  return !is_locked.exchange(true, std::memory_order_acquire);
}

// Release the lock. Assumes that the lock is acquired.
void SpinLock::Release(void) {
  is_locked.store(false, std::memory_order_release);
}

// Read-side acquire.
void ReaderWriterLock::ReadAcquire(void) {
  uint32_t old_value(0);
  for (;;) {
    old_value = lock.load(std::memory_order_acquire) & 0x7fffffffU;
    if (lock.compare_exchange_weak(
        old_value, old_value + 1, std::memory_order_release)) {
      return;
    }
    arch::Relax();
  }
}

// Read-side release.
void ReaderWriterLock::ReadRelease(void) {
  lock.fetch_sub(1, std::memory_order_release);
}

// Write-side acquire.
void ReaderWriterLock::WriteAcquire(void) {
  uint32_t old_value(0);

  for (;;) {
    old_value = lock.load(std::memory_order_acquire) & 0x7fffffffU;
    if (lock.compare_exchange_weak(
        old_value, old_value | 0x80000000, std::memory_order_release)) {
      break;
    }
    arch::Relax();
  }

  while (lock.load(std::memory_order_acquire) & 0x7fffffff) {
    arch::Relax();
  }
}

// Write-side release.
void ReaderWriterLock::WriteRelease(void) {
  lock.store(0, std::memory_order_release);
}

}  // namespace granary
