/**
 * @file Kernel/Memory/MemoryRegion.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryRegion.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MemoryMapping.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Represents an occupied region of memory.
   *
   * Unlike @ref MemoryMapping, this type tracks a single @ref AddressBlock
   * together with its @ref MemoryMappingFlags and @ref MemoryRegionType.
   */
  struct MemoryRegion {
    /**
     * @brief The @ref MemoryBlock that this region occupies.
     */
    MemoryBlock Block;

    /**
     * @brief The @ref MemoryMappingFlags that apply to this region.
     */
    MemoryMappingFlags Flags;

    /**
     * @brief The @ref MemoryRegionType that describes the purpose of this
     *        region.
     */
    MemoryRegionType RegionType = MemoryRegionType::None;

    /**
     * @brief Gets the keying @ref MemoryBlock for this region.
     * @return The @ref MemoryBlock that serves as the key for this region.
     */
    inline const MemoryBlock& GetBlock() const {
      return Block;
    }

    /**
     * @brief Compares two @ref MemoryRegion by their base address.
     * @param lhs The left-hand side @ref MemoryRegion.
     * @param rhs The right-hand side @ref MemoryRegion.
     * @return `true` if the left @ref MemoryRegion base is less than the
     *         right's.
     */
    friend constexpr bool operator<(
      MemoryRegion lhs,
      MemoryRegion rhs
    ) {
      return lhs.Block.Base < rhs.Block.Base;
    }
  };
}
