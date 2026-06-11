/**
 * @file Bootloader/KernelRuntime.cpp
 * @brief C++ runtime support for the bootloader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/Types.hpp>

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
    while (true) {}
  }

  /**
   * @brief Static destructor registration is a no-operation in the bootloader.
   * @return Always returns 0.
   */
  int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
  }

  /**
   * @brief DSO handle required by some toolchains.
   */
  void* __dso_handle = nullptr;
}

/**
 * @brief Global sized `delete` operator (no-operation in the bootloader).
 * @param ptr Pointer to memory (ignored).
 */
void operator delete(void* /*ptr*/, unsigned int /*size*/) noexcept {}

/**
 * @brief Global `delete` operator (no-operation in the bootloader).
 * @param ptr Pointer to memory (ignored).
 */
void operator delete(void* /*ptr*/) noexcept {}

/**
 * @brief Sets a block of memory to a specified value.
 * @param dest Destination memory block.
 * @param value Value to set.
 * @param count Number of bytes to set.
 * @return Pointer to the destination memory block.
 */
extern "C" void* memset(void* dest, int value, unsigned int count) {
  auto* bytes = reinterpret_cast<UInt8*>(dest);

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
  auto* out = reinterpret_cast<UInt8*>(dest);
  auto* in = reinterpret_cast<const UInt8*>(src);

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
 *   Zero if the blocks are equal; a negative value if left < right;
 *   a positive value if left > right.
 */
extern "C" int memcmp(const void* left, const void* right, unsigned int count) {
  auto* a = reinterpret_cast<const UInt8*>(left);
  auto* b = reinterpret_cast<const UInt8*>(right);

  for (unsigned int i = 0; i < count; ++i) {
    if (a[i] != b[i]) {
      return static_cast<int>(a[i]) - static_cast<int>(b[i]);
    }
  }

  return 0;
}
