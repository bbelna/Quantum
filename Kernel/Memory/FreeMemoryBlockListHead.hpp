/**
 * @file Kernel/Memory/FreeMemoryBlockListHead.hpp
 * @brief Declares @ref @QKrnl::Memory::FreeMemoryBlockListHead.
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
   * @brief Head/tail pointers and count for one order's free list.
   * @tparam Address The address type (e.g. `UInt32` for IA-32, `UInt64` for
   *                 x86-64).
   *
   * Lists are maintained in descending address order: @ref First is the
   * highest address (used for top-down allocation) and @ref Last is the lowest
   * (used for bottom-up allocation).
   */
  template<typename Address>
  struct FreeMemoryBlockListHead {
    /**
     * @brief Address of the first (highest-address) free block, or `0` if the
     *        list is empty.
     */
    Address First = 0;

    /**
     * @brief Address of the last (lowest-address) free block, or `0` if the
     *        list is empty.
     */
    Address Last = 0;

    /**
     * @brief Number of free blocks at this order.
     */
    UInt32 Count = 0;
  };
}
