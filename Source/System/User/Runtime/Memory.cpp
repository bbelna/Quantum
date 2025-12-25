/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/User/Runtime/Memory.cpp
 * User-mode memory helpers.
 */

#include <Types.hpp>

extern "C" void* memset(void* dest, int value, unsigned int count) {
  auto* bytes = reinterpret_cast<UInt8*>(dest);

  for (unsigned int i = 0; i < count; ++i) {
    bytes[i] = static_cast<UInt8>(value);
  }

  return dest;
}
