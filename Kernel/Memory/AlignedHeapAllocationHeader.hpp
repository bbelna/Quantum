/**
 * @file Kernel/Memory/AlignedHeapAllocationHeader.hpp
 * @brief Declares @ref @QKrnl::Memory::AlignedHeapAllocationHeader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FreeHeapBlock.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Metadata stored immediately before an aligned heap payload.
   */
  struct AlignedHeapAllocationHeader {
    /**
     * @brief Alignment marker to detect metadata.
     */
    UInt32 Magic;

    /**
     * @brief Owning free-block header for the allocation.
     */
    FreeHeapBlock* Block;

    /**
     * @brief Offset from the start of the block payload to the aligned heap
     *        address.
     */
    Size PayloadOffset;
  };
}
