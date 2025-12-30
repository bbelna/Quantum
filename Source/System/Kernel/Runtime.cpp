/**
 * @file System/Kernel/Runtime.cpp
 * @brief C++ runtime support.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Heap.hpp"
#include "Runtime.hpp"

extern "C" {
  /**
   * Thread-safe acquire guard for function-local statics.
   * @param guard
   *   Guard byte.
   * @return
   *   Non-zero if the guard is acquired; zero otherwise.
   */
  int __cxa_guard_acquire(unsigned char* guard) {
    return *guard == 0;
  }

  /**
   * Thread-safe release guard for function-local statics.
   * @param guard
   *   Guard byte.
   */
  void __cxa_guard_release(unsigned char* guard) {
    *guard = 1;
  }

  /**
   * Thread-safe abort guard for function-local statics.
   * @param guard
   *   Guard byte.
   */
  void __cxa_guard_abort(unsigned char* guard) {
    *guard = 0;
  }

  /**
   * Static destructor registration is a no-op in the kernel.
   * @return
   *   Always returns 0.
   */
  int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
  }

  /**
   * DSO handle required by some toolchains.
   */
  void* __dso_handle = nullptr;
}

/**
 * Global `new` operator using the kernel heap.
 * @param size
 *   Number of bytes to allocate.
 * @return
 *   Pointer to allocated memory.
 */
void* operator new(Size size) {
  return Quantum::System::Kernel::Heap::Allocate(size);
}

/**
 * Global `new[]` operator using the kernel heap.
 * @param size
 *   Number of bytes to allocate.
 * @return
 *   Pointer to allocated memory.
 */
void* operator new[](Size size) {
  return Quantum::System::Kernel::Heap::Allocate(size);
}

/**
 * Global `delete` operator using the kernel heap.
 * @param ptr
 *   Pointer to memory to free.
 */
void operator delete(void* ptr, unsigned int /*size*/) noexcept {
  Quantum::System::Kernel::Heap::Free(ptr);
}

/**
 * Global `delete` operator using the kernel heap.
 * @param ptr
 *   Pointer to memory to free.
 */
void operator delete(void* ptr) noexcept {
  Quantum::System::Kernel::Heap::Free(ptr);
}

/**
 * Global `delete[]` operator using the kernel heap.
 * @param ptr
 *   Pointer to memory to free.
 */
void operator delete[](void* ptr) noexcept {
  Quantum::System::Kernel::Heap::Free(ptr);
}

/**
 * Global `delete[]` operator using the kernel heap.
 * @param ptr
 *   Pointer to memory to free.
 */
void operator delete[](void* ptr, unsigned int /*size*/) noexcept {
  Quantum::System::Kernel::Heap::Free(ptr);
}

/**
 * Placement `new` operator.
 * @param size
 *   Size of the allocation (ignored).
 * @param place
 *   Placement address.
 * @return
 *   The placement address.
 */
void* operator new(Size /*size*/, void* place) noexcept {
  return place;
}

/**
 * Placement `delete` operator (no-op).
 * @param ptr
 *   Pointer to memory (ignored).
 * @param place
 *   Placement address (ignored).
 */
void operator delete(void* /*ptr*/, void* /*place*/) noexcept {}

namespace Quantum::System::Kernel {
  /**
   * Initialization function type for global constructors.
   */
  using InitFunction = void (*)();

  extern "C" {
    /**
     * Start of the global constructors array.
     */
    extern InitFunction __init_array_start[];

    /**
     * End of the global constructors array.
     */
    extern InitFunction __init_array_end[];
  }

  /**
   * Runs global constructors.
   */
  void RunGlobalConstructors() {
    for (InitFunction* fn = __init_array_start; fn < __init_array_end; ++fn) {
      if (*fn) {
        (*fn)();
      }
    }
  }
}
