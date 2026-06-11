/**
 * @file Kernel/Memory/SharedBufferRepository.hpp
 * @brief Declares @ref @QKrnl::Memory::SharedBufferRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelConstants.hpp>

#include "ObjectPool.hpp"
#include "SharedBuffer.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Repository for @ref SharedBuffer.
   * @note All public methods are called from ABI / system call handlers with
   *       interrupts disabled, so no additional locking is required at this
   *       time.
   */
  class SharedBufferRepository {
    public:
      /**
       * @brief Creates a new @ref SharedBufferRepository.
       * @param sharedBufferPool Reference to the @ref SharedBuffer
       *                         @ref ObjectPool.
       */
      SharedBufferRepository(
        ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>& sharedBufferPool
      );

      /**
       * @brief
       *   Creates and persists a new @ref SharedBuffer with the given
       *   @ref MemoryBlock array.
       * @note
       *   The repository takes ownership of the @ref blocks array.
       * @param blocks
       *   Heap-allocated array of @ref MemoryBlock (caller transfers
       *   ownership).
       * @param blockCount
       *   Number of entries in @ref blocks.
       * @param sizeInBytes
       *   Original requested size in bytes.
       * @return
       *   Pointer to the new @ref SharedBuffer.
       */
      SharedBuffer* Create(
        MemoryBlock* blocks,
        Size blockCount,
        Size sizeInBytes
      );

      /**
       * @brief
       *   Finds a persisted @ref SharedBuffer by its @ref SharedBufferID.
       * @param id
       *   The @ref SharedBufferID to look up.
       * @return
       *   Pointer to the @ref SharedBuffer, or `nullptr` if not found.
       */
      SharedBuffer* Find(SharedBufferID id);

      /**
       * @brief
       *   Removes a persisted @ref SharedBuffer from the repository and frees
       *   its kernel-side resources (@ref SharedBuffer::Blocks array and
       *   @ref SharedBuffer object).
       * @note
       *   Does not unmap @ref MemoryBlock from any @ref IAddressSpace;
       *   the caller is responsible for unmapping before calling @ref Remove.
       * @param id
       *   The @ref SharedBufferID of the buffer to remove.
       */
      void Remove(SharedBufferID id);

      /**
       * @brief Returns the number of persisted @ref SharedBuffer in the
       *        repository.
       * @return The persisted @ref SharedBuffer count.
       */
      Size GetCount() const { return _count; }

    private:
      /**
       * @brief Reference to the @ref SharedBuffer @ref ObjectPool.
       */
      ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>& _sharedBufferPool;

      /**
       * @brief @ref LinkedList of pointers to persisted @ref SharedBuffer.
       */
      LinkedList<SharedBuffer*> _buffers;

      /**
       * @brief Monotonically increasing ID counter.
       * @note Starts at `1` so that `0` can be used as a sentinel
       *       "no shared buffer" value.
       */
      SharedBufferID _nextID = 1;

      /**
       * @brief The count of persisted @ref SharedBuffer.
       */
      Size _count = 0;
  };
}
