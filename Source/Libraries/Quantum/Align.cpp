/**
 * @file Libraries/Quantum/Align.cpp
 * @brief Alignment helper utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Align.hpp"

namespace Quantum {
  UInt32 AlignUp(UInt32 value, UInt32 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  UInt32 AlignDown(UInt32 value, UInt32 alignment) {
    return value & ~(alignment - 1);
  }
}
