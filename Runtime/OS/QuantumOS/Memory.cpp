/**
 * @file Runtime/QuantumMemory.cpp
 * @brief Runtime memory manipulation functions for QuantumOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "QuantumOSRuntimeTypes.hpp"

/**
 * @brief Sets a block of memory to a specified value.
 * @param destination Destination memory block.
 * @param value Value to set.
 * @param count Number of bytes to set.
 * @return Pointer to the destination memory block.
 */
extern "C" void* memset(
  void* destination,
  int value,
  unsigned int count
) {
  UInt8* bytes = reinterpret_cast<UInt8*>(destination);

  for (unsigned int index = 0; index < count; ++index) {
    bytes[index] = static_cast<UInt8>(value);
  }

  return destination;
}

/**
 * @brief Copies a block of memory from source to destination.
 * @param destination Destination memory block.
 * @param source Source memory block.
 * @param count Number of bytes to copy.
 * @return Pointer to the destination memory block.
 */
extern "C" void* memcpy(
  void* destination,
  const void* source,
  unsigned int count
) {
  UInt8* out = reinterpret_cast<UInt8*>(destination);
  const UInt8* in = reinterpret_cast<const UInt8*>(source);

  for (unsigned int index = 0; index < count; ++index) {
    out[index] = in[index];
  }

  return destination;
}

/**
 * @brief Compares two blocks of memory.
 * @param left First memory block.
 * @param right Second memory block.
 * @param count Number of bytes to compare.
 * @return Zero if the blocks are equal; a negative value if left < right;
 *   a positive value if left > right.
 */
extern "C" int memcmp(
  const void* left,
  const void* right,
  unsigned int count
) {
  const UInt8* leftAsBytes = reinterpret_cast<const UInt8*>(left);
  const UInt8* rightAsBytes = reinterpret_cast<const UInt8*>(right);

  for (unsigned int index = 0; index < count; ++index) {
    if (leftAsBytes[index] != rightAsBytes[index]) {
      return
        static_cast<int>(leftAsBytes[index]) -
        static_cast<int>(rightAsBytes[index]);
    }
  }

  return 0;
}
