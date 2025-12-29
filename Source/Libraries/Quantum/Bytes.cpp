/**
 * @file Libraries/Quantum/Bytes.cpp
 * @brief Byte manipulation helper utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Bytes.hpp"

namespace Quantum {
  void CopyBytes(void* destination, const void* source, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(destination);
    auto* s = reinterpret_cast<const UInt8*>(source);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }
}
