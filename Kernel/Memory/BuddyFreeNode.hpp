/**
 * @file Kernel/Memory/BuddyFreeNode.hpp
 * @brief Declares @ref @QKrnl::Memory::BuddyFreeNode.
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
   * @brief Node embedded in the first bytes of every free block.
   * @tparam Address The address type (e.g. `UInt32` for IA-32, `UInt64` for
   *                 x86-64).
   *
   * Because the block is free, its memory is not in use and can store
   * the linked-list pointers. This avoids separate metadata allocation
   * for the free lists.
   */
  template<typename Address>
  struct BuddyFreeNode {
    /**
     * @brief Address of the next free block in this order's list, or `0` if
     *        this is the tail.
     */
    Address Next;

    /**
     * @brief Address of the previous free block in this order's list, or `0` if
     *        this is the head.
     */
    Address Previous;
  };
}
