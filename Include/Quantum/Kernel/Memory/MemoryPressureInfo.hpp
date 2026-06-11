/**
 * @file Include/Quantum/Kernel/Memory/MemoryPressureInfo.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryPressureInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MemoryPressureState.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Memory pressure info.
   */
  struct MemoryPressureInfo {
    /**
     * @brief Total number of physical memory blocks.
     */
    Size TotalBlocks;

    /**
     * @brief Number of free physical memory blocks.
     */
    Size FreeBlocks;

    /**
     * @brief Number of used physical memory blocks.
     */
    Size UsedBlocks;

    /**
     * @brief Physical memory mapped for the kernel heap, in bytes.
     */
    Size KernelHeapBytes;

    /**
     * @brief Number of active shared buffer descriptors.
     */
    Size SharedBufferCount;

    /**
     * @brief Memory consumed by the IPC message pool, in bytes.
     */
    Size PoolIPCMessageBytes;

    /**
     * @brief Memory consumed by the IPC port pool, in bytes.
     */
    Size PoolIPCPortBytes;

    /**
     * @brief Memory consumed by the shared buffer pool, in bytes.
     */
    Size PoolSharedBufferBytes;

    /**
     * @brief Current memory pressure state.
     */
    MemoryPressureState State;
  };
}
