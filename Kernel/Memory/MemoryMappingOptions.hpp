/**
 * @file Kernel/Memory/MemoryMappingOptions.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryMappingOptions.
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
   * @brief Options for mapping @ref MemoryBlock.
   */
  enum class MemoryMappingOptions : UInt32 {
    /**
     * @brief No special options.
     */
    None = 0,

    /**
     * @brief Mark the mapping as global (shared across all address spaces).
     */
    Global = 1u << 0,

    /**
     * @brief Mark the mapped block as a guard page/block. Access to a guard
     *        block triggers a fault used to detect stack overflow or heap
     *        overrun.
     */
    Guard = 1u << 1,

    /**
     * @brief Do not trigger a page fault on mapping failures.
     */
    NoFault = 1u << 2,

    /**
     * @brief The region is lazily allocated: recorded in the address space
     *        map but has no kernel backing yet. A page fault on this
     *        region triggers on-demand allocation.
     */
    Lazy = 1u << 3
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Kernel::Memory, MemoryMappingOptions
)
