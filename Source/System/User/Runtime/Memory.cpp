/**
 * @file System/User/Runtime/Memory.cpp
 * @brief User-mode memory functions.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

/**
 * Sets a block of memory to a specified value.
 * @param dest
 *   Destination memory block.
 * @param value
 *   Value to set.
 * @param count
 *   Number of bytes to set.
 * @return
 *   Pointer to the destination memory block.
 */
extern "C" void* memset(void* dest, int value, unsigned int count) {
  auto* bytes = reinterpret_cast<UInt8*>(dest);

  for (unsigned int i = 0; i < count; ++i) {
    bytes[i] = static_cast<UInt8>(value);
  }

  return dest;
}

/**
 * Copies a block of memory from source to destination.
 * @param dest
 *   Destination memory block.
 * @param src
 *   Source memory block.
 * @param count
 *   Number of bytes to copy.
 * @return
 *   Pointer to the destination memory block.
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
 * Compares two blocks of memory.
 * @param left
 *   First memory block.
 * @param right
 *   Second memory block.
 * @param count
 *   Number of bytes to compare.
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
