/**
 * @file System/User/Runtime/Memory.cpp
 * @brief User-mode memory functions.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

extern "C" void* memset(void* dest, int value, unsigned int count) {
  auto* bytes = reinterpret_cast<UInt8*>(dest);

  for (unsigned int i = 0; i < count; ++i) {
    bytes[i] = static_cast<UInt8>(value);
  }

  return dest;
}

extern "C" void* memcpy(void* dest, const void* src, unsigned int count) {
  auto* out = reinterpret_cast<UInt8*>(dest);
  auto* in = reinterpret_cast<const UInt8*>(src);

  for (unsigned int i = 0; i < count; ++i) {
    out[i] = in[i];
  }

  return dest;
}

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
