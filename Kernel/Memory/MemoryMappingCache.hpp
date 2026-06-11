/**
 * @file Kernel/Memory/MemoryMappingCache.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryMappingCache.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Caching strategies for mapping @ref MemoryBlock.
   */
  enum class MemoryMappingCache : UInt32 {
    /**
     * @brief Default caching strategy.
     */
    Default = 0,

    /**
     * @brief Write-through caching strategy.
     */
    WriteThrough = 1u << 0,

    /**
     * @brief Write-back caching strategy.
     */
    WriteBack = 1u << 1,

    /**
     * @brief Uncached strategy.
     */
    Uncached = 1u << 2,

    /**
     * @brief Write-combining strategy. Batches sequential writes for
     *        high-throughput framebuffer access. Falls back to Uncached
     *        if the CPU does not support PAT.
     */
    WriteCombining = 1u << 3
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Kernel::Memory, MemoryMappingCache
)
