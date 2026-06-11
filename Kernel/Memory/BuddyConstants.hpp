/**
 * @file Kernel/Memory/BuddyConstants.hpp
 * @brief Declares @ref @QKrnl::Memory buddy constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Maximum buddy order. Order 0 = 1 block, order 20 = 2^20 blocks.
   */
  constexpr UInt8 MaxBuddyOrder = 20;

  /**
   * @brief Sentinel value stored in the per-block order array for an
   *        allocated block.
   */
  constexpr UInt8 OrderAllocated = 0xFD;

  /**
   * @brief Sentinel value stored in the per-block order array for a
   *        reserved block (kernel image, metadata, boot stack, etc.).
   */
  constexpr UInt8 OrderReserved = 0xFE;

  /**
   * @brief Sentinel value stored in the per-block order array for a
   *        non-base sub-block of a larger buddy.
   */
  constexpr UInt8 OrderSentinel = 0xFF;

  /**
   * @brief Temporary marker used during initialization to flag usable
   *        blocks before the buddy tree is built.
   */
  constexpr UInt8 OrderPendingFree = 0xFC;

  /**
   * @brief Returns the smallest k such that (1 << k) >= @p value.
   * @param value The value to compute the ceiling log2 of. Must be > 0.
   * @return The ceiling of log2(@p value).
   */
  inline UInt8 CeilLog2(UInt32 value) {
    if (value <= 1) return 0;

    UInt32 v = value - 1;
    UInt8 result = 0;

    while (v > 0) {
      v >>= 1;
      ++result;
    }

    return result;
  }
}
