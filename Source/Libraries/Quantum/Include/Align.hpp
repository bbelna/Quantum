/**
 * @file Libraries/Quantum/Include/Align.hpp
 * @brief Alignment helper utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "Types.hpp"

namespace Quantum {
  /**
   * Aligns a value up to the next multiple of alignment.
   * @param value
   *   Value to align.
   * @param alignment
   *   Alignment boundary (power of two).
   * @return
   *   Aligned value.
   */
  UInt32 AlignUp(UInt32 value, UInt32 alignment);

  /**
   * Aligns a value down to the nearest alignment boundary.
   * @param value
   *   Value to align.
   * @param alignment
   *   Alignment boundary (power of two).
   * @return
   *   Aligned value at or below input.
   */
  UInt32 AlignDown(UInt32 value, UInt32 alignment);
}
