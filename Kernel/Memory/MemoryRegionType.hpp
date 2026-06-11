/**
 * @file Kernel/Memory/MemoryRegionType.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryRegionType.
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
   * @brief Describes the semantic type of a @ref MemoryRegion.
   */
  enum class MemoryRegionType : UInt8 {
    /**
     * @brief No region type assigned.
     */
    None = 0,

    /**
     * @brief Executable code region (e.g., `.text` section).
     */
    Code = 1,

    /**
     * @brief Initialized or uninitialized data region (e.g., `.data`, `.bss`).
     */
    Data = 2,

    /**
     * @brief Dynamically allocated heap region.
     */
    Heap = 3,

    /**
     * @brief Thread stack region.
     */
    Stack = 4,

    /**
     * @brief Guard page/block placed around stacks or heaps to detect overflow.
     */
    Guard = 5,

    /**
     * @brief Shared memory region backed by a @ref SharedBuffer.
     */
    Shared = 6,

    /**
     * @brief Memory-mapped device I/O region (e.g., VRAM, MMIO registers).
     */
    Device = 7
  };
}
