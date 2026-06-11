/**
 * @file Kernel/Memory/SharedBuffer.hpp
 * @brief Declares @ref @QKrnl::Memory::SharedBuffer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "MemoryMappingCache.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Shared kernel memory buffer.
   *
   * Stores an ordered heap-allocated array of @ref MemoryBlock
   * instances that back the shared region. By keeping the @ref MemoryBlock
   * list here, any @ref Process can attach the buffer via
   * `Memory_AttachShared` without the creator @ref Process needing to
   * remain alive at attach time.
   */
  struct SharedBuffer {
    /**
     * @brief @ref SharedBufferID for this buffer.
     */
    SharedBufferID ID = 0;

    /**
     * @brief Heap-allocated array of @ref MemoryBlock backing this buffer.
     * @note Freed when the buffer is removed from the
     *       @ref SharedBufferRepository.
     */
    MemoryBlock* Blocks = nullptr;

    /**
     * @brief Number of entries in @ref Blocks.
     */
    Size BlockCount = 0;

    /**
     * @brief Original requested size in bytes.
     */
    Size SizeInBytes = 0;

    /**
     * @brief Number of active mappings (attached processes).
     * @note The buffer is removed from the @ref SharedBufferRepository
     *       when this reaches zero (unless @ref Pinned is `true`).
     */
    Size AttachCount = 0;

    /**
     * @brief When `true`, @ref Blocks point to device memory (e.g. VRAM)
     *        that is NOT tracked by the @ref @QKrnl
     *        @ref IMemoryAllocator.
     * @note `Retain`/`Free` are skipped in attach/detach, and the buffer
     *       is never removed from the @ref SharedBufferRepository (it lives
     *       until shutdown).
     */
    bool Pinned = false;

    /**
     * @brief Cache policy applied when `Memory_AttachShared` maps pages
     *        into a @ref Process address space.
     * @note `Default` for normal RAM buffers; `WriteCombining` for VRAM
     *       framebuffers.
     */
    MemoryMappingCache CacheMode = MemoryMappingCache::Default;
  };
}
