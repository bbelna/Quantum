/**
 * @file Kernel/KernelRuntime.cpp
 * @brief Implements @ref @QKrnl C++ runtime support.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Kernel.hpp"
#include "Memory/HeapAllocator.hpp"

extern "C" {
  /**
   * @brief Thread-safe acquire guard for function-local statics.
   * @param guard Guard byte.
   * @return Non-zero if the guard is acquired; zero otherwise.
   */
  int __cxa_guard_acquire(unsigned char* guard) {
    return *guard == 0;
  }

  /**
   * @brief Thread-safe release guard for function-local statics.
   * @param guard Guard byte.
   */
  void __cxa_guard_release(unsigned char* guard) {
    *guard = 1;
  }

  /**
   * @brief Thread-safe abort guard for function-local statics.
   * @param guard Guard byte.
   */
  void __cxa_guard_abort(unsigned char* guard) {
    *guard = 0;
  }

  /**
   * @brief Pure virtual function handler.
   */
  void __cxa_pure_virtual() {
    // no handler available in pure virtual context, halt directly
    #if defined (ARCH_IA32)
    while (true) {
      asm volatile(
        "cli\n"
        "hlt\n"
      );
    }
    #endif
  }

  /**
   * @brief Static destructor registration is a no-operation in the kernel.
   * @return Always returns `0`.
   */
  int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
  }

  /**
   * @brief DSO handle required by some toolchains.
   */
  void* __dso_handle = nullptr;

  /**
   * @brief 64-bit unsigned division for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The quotient.
   */
  UInt64 __udivdi3(UInt64 dividend, UInt64 divisor) {
    if (divisor == 0) return 0;

    UInt64 quotient = 0;
    UInt64 remainder = 0;

    for (int i = 63; i >= 0; i--) {
      remainder = (remainder << 1) | ((dividend >> i) & 1);

      if (remainder >= divisor) {
        remainder -= divisor;
        quotient |= (1ULL << i);
      }
    }

    return quotient;
  }

  /**
   * @brief 64-bit unsigned modulo for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The remainder.
   */
  UInt64 __umoddi3(UInt64 dividend, UInt64 divisor) {
    if (divisor == 0) return 0;

    UInt64 remainder = 0;

    for (int i = 63; i >= 0; i--) {
      remainder = (remainder << 1) | ((dividend >> i) & 1);

      if (remainder >= divisor) {
        remainder -= divisor;
      }
    }

    return remainder;
  }

  /**
   * @brief 64-bit signed division for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The quotient.
   */
  Int64 __divdi3(Int64 dividend, Int64 divisor) {
    bool negative = false;

    if (dividend < 0) {
      negative = !negative;
      dividend = -dividend;
    }

    if (divisor < 0) {
      negative = !negative;
      divisor = -divisor;
    }

    UInt64 result = __udivdi3(
      static_cast<UInt64>(dividend),
      static_cast<UInt64>(divisor)
    );

    return negative
      ? -static_cast<Int64>(result)
      : static_cast<Int64>(result);
  }

  /**
   * @brief 64-bit signed modulo for 32-bit targets.
   * @param dividend The dividend.
   * @param divisor The divisor.
   * @return The remainder.
   */
  Int64 __moddi3(Int64 dividend, Int64 divisor) {
    bool negative = dividend < 0;

    if (dividend < 0) dividend = -dividend;
    if (divisor < 0) divisor = -divisor;

    UInt64 result = __umoddi3(
      static_cast<UInt64>(dividend),
      static_cast<UInt64>(divisor)
    );

    return negative
      ? -static_cast<Int64>(result)
      : static_cast<Int64>(result);
  }
}

/**
 * @brief Global `new` operator using the kernel heap.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory.
 */
void* operator new(Size size) {
  return Context->HeapAllocator->Allocate(size);
}

/**
 * @brief Global `new[]` operator using the kernel heap.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory.
 */
void* operator new[](Size size) {
  return Context->HeapAllocator->Allocate(size);
}

/**
 * @brief Global `delete` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 */
void operator delete(void* ptr, unsigned int /*size*/) noexcept {
  Context->HeapAllocator->Free(ptr);
}

/**
 * @brief Global `delete` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 */
void operator delete(void* ptr) noexcept {
  Context->HeapAllocator->Free(ptr);
}

/**
 * @brief Global `delete[]` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 */
void operator delete[](void* ptr) noexcept {
  Context->HeapAllocator->Free(ptr);
}

/**
 * @brief Global `delete[]` operator using the kernel heap.
 * @param ptr Pointer to memory to free.
 */
void operator delete[](void* ptr, unsigned int /*size*/) noexcept {
  Context->HeapAllocator->Free(ptr);
}

/**
 * @brief Placement `new` operator.
 * @param size Size of the allocation (ignored).
 * @param place Placement address.
 * @return The placement address.
 */
void* operator new(Size /*size*/, void* place) noexcept {
  return place;
}

/**
 * @brief Placement `delete` operator (no-operation).
 * @param ptr Pointer to memory (ignored).
 * @param place Placement address (ignored).
 */
void operator delete(void* /*ptr*/, void* /*place*/) noexcept {}

/**
 * @brief Sets a block of memory to a specified value.
 * @param dest Destination memory block.
 * @param value Value to set.
 * @param count Number of bytes to set.
 * @return Pointer to the destination memory block.
 */
extern "C" void* memset(void* dest, int value, unsigned int count) {
  UInt8* bytes = reinterpret_cast<UInt8*>(dest);

  for (unsigned int i = 0; i < count; ++i) {
    bytes[i] = static_cast<UInt8>(value);
  }

  return dest;
}

/**
 * @brief Copies a block of memory from source to destination.
 * @param dest Destination memory block.
 * @param src Source memory block.
 * @param count Number of bytes to copy.
 * @return Pointer to the destination memory block.
 */
extern "C" void* memcpy(void* dest, const void* src, unsigned int count) {
  UInt8* out = reinterpret_cast<UInt8*>(dest);
  const UInt8* in = reinterpret_cast<const UInt8*>(src);

  for (unsigned int i = 0; i < count; ++i) {
    out[i] = in[i];
  }

  return dest;
}

/**
 * @brief Compares two blocks of memory.
 * @param left First memory block.
 * @param right Second memory block.
 * @param count Number of bytes to compare.
 * @return
 *   `0` if the blocks are equal; a negative value if `left < right`;
 *   a positive value if `left > right`.
 */
extern "C" int memcmp(const void* left, const void* right, unsigned int count) {
  const UInt8* a = reinterpret_cast<const UInt8*>(left);
  const UInt8* b = reinterpret_cast<const UInt8*>(right);

  for (unsigned int i = 0; i < count; ++i) {
    if (a[i] != b[i]) {
      return static_cast<int>(a[i]) - static_cast<int>(b[i]);
    }
  }

  return 0;
}
