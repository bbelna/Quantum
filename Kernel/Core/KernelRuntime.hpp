/**
 * @file Kernel/KernelRuntime.hpp
 * @brief Declares @ref @QKrnl C++ runtime support.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "KernelTypes.hpp"

extern "C" {
  /**
   * @brief Thread-safe acquire guard for function-local statics.
   * @param guard Guard byte.
   * @return Non-zero if the guard is acquired; zero otherwise.
   */
  int __cxa_guard_acquire(unsigned char* guard);

  /**
   * @brief Thread-safe release guard for function-local statics.
   * @param guard Guard byte.
   */
  void __cxa_guard_release(unsigned char* guard);

  /**
   * @brief Thread-safe abort guard for function-local statics.
   * @param guard Guard byte.
   */
  void __cxa_guard_abort(unsigned char* guard);

  /**
   * @brief Pure virtual function handler.
   */
  void __cxa_pure_virtual();

  /**
   * @brief Static destructor registration is a no-operation in the kernel.
   * @return Always returns `0`.
   */
  int __cxa_atexit(void (*)(void*), void*, void*);

  /**
   * @brief DSO handle required by some toolchains.
   */
  extern void* __dso_handle;

  /**
   * @brief 64-bit unsigned division for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The quotient.
   */
  UInt64 __udivdi3(UInt64 dividend, UInt64 divisor);

  /**
   * @brief 64-bit unsigned modulo for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The remainder.
   */
  UInt64 __umoddi3(UInt64 dividend, UInt64 divisor);

  /**
   * @brief 64-bit signed division for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The quotient.
   */
  Int64 __divdi3(Int64 dividend, Int64 divisor);

  /**
   * @brief 64-bit signed modulo for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The remainder.
   */
  Int64 __moddi3(Int64 dividend, Int64 divisor);

  /**
   * @brief Sets a block of memory to a specified value.
   * @param dest Destination memory block.
   * @param value Value to set.
   * @param count Number of bytes to set.
   * @return Pointer to the destination memory block.
   */
  void* memset(void* dest, int value, unsigned int count);

  /**
   * @brief Copies a block of memory from source to destination.
   * @param dest Destination memory block.
   * @param src Source memory block.
   * @param count Number of bytes to copy.
   * @return Pointer to the destination memory block.
   */
  void* memcpy(void* dest, const void* src, unsigned int count);

  /**
   * @brief Compares two blocks of memory.
   * @param left First memory block.
   * @param right Second memory block.
   * @param count Number of bytes to compare.
   * @return
   *   `0` if the blocks are equal; a negative value if `left < right`;
   *   a positive value if `left > right`.
   */
  int memcmp(const void* left, const void* right, unsigned int count);
}

/**
 * @brief Global `new` operator using the kernel heap.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory.
 */
void* operator new(Size size);

/**
 * @brief Global `new[]` operator using the kernel heap.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory.
 */
void* operator new[](Size size);

/**
 * @brief Global sized `delete` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 * @param size Size of the allocation (ignored).
 */
void operator delete(void* ptr, unsigned int size) noexcept;

/**
 * @brief Global `delete` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 */
void operator delete(void* ptr) noexcept;

/**
 * @brief Global `delete[]` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 */
void operator delete[](void* ptr) noexcept;

/**
 * @brief Global sized `delete[]` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 * @param size Size of the allocation (ignored).
 */
void operator delete[](void* ptr, unsigned int size) noexcept;

/**
 * @brief Placement `new` operator.
 * @param size Size of the allocation (ignored).
 * @param place Placement address.
 * @return The placement address.
 */
void* operator new(Size size, void* place) noexcept;

/**
 * @brief Placement `delete` operator (no-operation).
 * @param ptr Pointer to memory (ignored).
 * @param place Placement address (ignored).
 */
void operator delete(void* ptr, void* place) noexcept;
