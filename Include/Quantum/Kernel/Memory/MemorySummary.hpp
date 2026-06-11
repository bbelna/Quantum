/**
 * @file Include/Quantum/Kernel/Memory/MemorySummary.hpp
 * @brief Declares @ref @QKrnl::Memory::MemorySummary.
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
   * @brief Summary of the system's memory usage.
   */
  struct MemorySummary {
    /**
     * @brief Total memory under management in bytes.
     */
    Size TotalBytes;

    /**
     * @brief Total used memory in bytes.
     */
    Size UsedBytes;

    /**
     * @brief Total free memory in bytes.
     */
    Size FreeBytes;

    /**
     * @brief Size of a single @ref MemoryBlock in bytes.
     */
    Size BlockSize;

    /**
     * @brief Total number of @ref MemoryBlock instances.
     */
    Size BlockCount;

    /**
     * @brief Number of used @ref MemoryBlock instances.
     */
    Size UsedBlockCount;

    /**
     * @brief Size occupied by the initial image in bytes.
     */
    Size InitialImageBytes;

    /**
     * @brief Memory reserved in bytes.
     */
    Size KernelReservedBytes;

    /**
     * @brief Memory mapped for the kernel heap in bytes.
     */
    Size KernelHeapBytes;

    /**
     * @brief Total memory usage attributed to user processes in bytes.
     */
    Size UserProcessBytes;
  };
}
