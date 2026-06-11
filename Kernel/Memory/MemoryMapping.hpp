/**
 * @file Kernel/Memory/MemoryMapping.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryMapping.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "MemoryMappingFlags.hpp"
#include "MemoryRegionType.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Represents a @ref MemoryBlock mapping.
   */
  struct MemoryMapping {
    /**
     * @brief The address space this mapping belongs to.
     */
    IAddressSpace* AddressSpace;

    /**
     * @brief The address block in the process' address space.
     */
    MemoryBlock ProcessBlock;

    /**
     * @brief The address block in the kernel's address space.
     */
    MemoryBlock KernelBlock;

    /**
     * @brief The mapping flags.
     */
    MemoryMappingFlags Flags;

    /**
     * @brief The semantic type of this memory region.
     */
    MemoryRegionType RegionType = MemoryRegionType::None;

    /**
     * @brief For `MemoryRegionType::Shared` mappings: the system-wide
     *        ID of the `SharedBuffer` kernel object backing this mapping.
     *        Zero for all other region types.
     */
    UInt32 SharedBufferID = 0;

    /**
     * @brief Returns the keying block for this mapping (the process-side
     *        block).  Required by @ref AddressSpaceMap.
     */
    inline const MemoryBlock& GetBlock() const { return ProcessBlock; }

    /**
     * @brief Compares two process address block mappings by their process
     *        block's base address.
     * @param lhs The left-hand side mapping.
     * @param rhs The right-hand side mapping.
     * @return `true` if the left mapping's process block base is less than the
     *         right's.
     */
    friend constexpr bool operator<(
      MemoryMapping lhs,
      MemoryMapping rhs
    ) {
      return lhs.ProcessBlock.Base < rhs.ProcessBlock.Base;
    }
  };
}
