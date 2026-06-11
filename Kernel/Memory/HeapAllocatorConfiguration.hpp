/**
 * @file Kernel/Memory/HeapAllocatorConfiguration.hpp
 * @brief Declares @ref @QKrnl::Memory::HeapAllocatorConfiguration.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Configuration parameters for initializing a @ref HeapAllocator.
   */
  struct HeapAllocatorConfiguration {
    /**
     * @brief The heap's address space.
     */
    IAddressSpace* HeapAddressSpace;

    /**
     * @brief The block size to use for heap block mappings.
     */
    MemoryBlock HeapBlock;

    /**
     * @brief Number of guard blocks to place before the heap.
     */
    Size GuardBlocksBefore;

    /**
     * @brief Number of guard blocks to place after the heap.
     */
    Size GuardBlocksAfter;

    /**
     * @brief The magic value to use for aligned allocation metadata.
     */
    UIntPtr AlignedMagic;

    /**
     * @brief The poison byte to write for allocated memory.
     */
    UInt8 AllocatedPoison;

    /**
     * @brief The poison byte to write for freed memory.
     */
    UInt8 FreedPoison;

    /**
     * @brief The canary value to write for free blocks.
     */
    UIntPtr Canary;

    /**
     * @brief The sentinel value to write for allocated blocks.
     */
    UIntPtr AllocatedSentinel;

    /**
     * @brief Minimum required tail blocks to keep free for the heap to satisfy
     *        future allocation requests.
     *
     * Note that @ref HeapAllocator will ensure that this value is never set
     * below @ref HeapAllocator::MinimumTailBlocks.
     */
    Size RequiredTailBlocks;
  };
}
