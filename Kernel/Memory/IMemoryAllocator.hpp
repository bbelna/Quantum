/**
 * @file Kernel/Memory/IMemoryAllocator.hpp
 * @brief Declares @ref @QKrnl::Memory::IMemoryAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "MemoryBlockTag.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Abstract interface for allocating and freeing @ref MemoryBlock.
   *
   * Architecture-specific implementations (e.g., the IA-32 bitmap allocator)
   * inherit from this interface. Kernel subsystems use @ref IMemoryAllocator
   * so that heap, stack, and mapping code remains architecture-independent.
   *
   * Blocks are reference-counted: @ref Retain increments the count and
   * @ref Free decrements it, releasing the block only when the count reaches
   * zero.
   */
  class IMemoryAllocator {
    public:
      /**
       * @brief Allocates a @ref MemoryBlock.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         if allocation fails.
       */
      virtual MemoryBlock Allocate() = 0;

      /**
       * @brief Allocates a @ref MemoryBlock of the given @p size.
       * @param size The size of the @ref MemoryBlock to allocate.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         if allocation fails.
       */
      virtual MemoryBlock Allocate(Size size) = 0;

      /**
       * @brief Allocates a single @ref MemoryBlock whose address is below the
       *        given @p limit.
       * @param limit The exclusive upper bound on the @ref MemoryBlock.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         if none is available below the limit.
       */
      virtual MemoryBlock AllocateBelow(UInt32 limit) = 0;

      /**
       * @brief Frees a @ref MemoryBlock.
       * @param block The @ref MemoryBlock to free.
       *
       * If the @ref MemoryBlock is reference-counted, decrements the reference
       * count and only releases the @ref MemoryBlock when the count reaches
       * zero.
       */
      virtual void Free(MemoryBlock block) = 0;

      /**
       * @brief Increments the reference count on a @ref MemoryBlock.
       * @param block The @ref MemoryBlock to retain.
       *
       * Call this when sharing a @ref MemoryBlock between multiple
       * @ref IAddressSpace (e.g., during process spawn). Each @ref Retain must
       * be balanced by a corresponding @ref Free.
       */
      virtual void Retain(MemoryBlock block) = 0;

      /**
       * @brief Gets the size of @ref MemoryBlock allocated.
       * @return The @ref MemoryBlock size in bytes.
       */
      virtual Size GetBlockSize() const = 0;

      /**
       * @brief Gets the total number of bytes under management.
       * @return The total managed bytes.
       */
      virtual Size GetManagedBytes() const = 0;

      /**
       * @brief Gets the total number of @ref MemoryBlock under management.
       * @return The total @ref MemoryBlock count.
       */
      virtual Size GetBlockCount() const = 0;

      /**
       * @brief Gets the number of used (allocated) @ref MemoryBlock.
       * @return The used @ref MemoryBlock count.
       */
      virtual Size GetUsedBlockCount() const = 0;

      /**
       * @brief Gets the total bytes reserved at boot.
       * @return The reserved byte count.
       */
      virtual Size GetReservedBytes() const { return 0; }

      /**
       * @brief Gets the bytes currently reserved for the initial image
       *        (startup bundle).
       * @return The initial image byte count.
       * @note Returns `0` after @ref FreeInitialImage.
       */
      virtual Size GetInitialImageBytes() const { return 0; }

      /**
       * @brief Allocates a single @ref MemoryBlock with the given
       *        @ref MemoryBlockTag.
       * @param tag The @ref MemoryBlockTag to associate with the new
       *            @ref MemoryBlock.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      virtual MemoryBlock Allocate(MemoryBlockTag tag) {
        return Allocate();
      }

      /**
       * @brief Allocates a contiguous @ref MemoryBlock with the given
       *        @ref MemoryBlockTag.
       * @param size The size in bytes (rounded up to @ref MemoryBlock size).
       * @param tag The @ref MemoryBlockTag to associate with the new
       *            @ref MemoryBlock.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      virtual MemoryBlock Allocate(Size size, MemoryBlockTag tag) {
        return Allocate(size);
      }

      /**
       * @brief Allocates a single @ref MemoryBlock below the given @p limit.
       * @param limit The exclusive upper bound on the @ref MemoryBlock.
       * @param tag The @ref MemoryBlockTag to associate with the new
       *            @ref MemoryBlock.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      virtual MemoryBlock AllocateBelow(
        UInt32 limit,
        MemoryBlockTag tag
      ) {
        return AllocateBelow(limit);
      }

      /**
       * @brief Returns the number of used @ref MemoryBlock for a given
       *        @ref MemoryBlockTag.
       * @param tag The @ref MemoryBlockTag to query.
       * @return The number of @ref MemoryBlock allocated with this
       *         @ref MemoryBlockTag.
       */
      virtual Size GetUsedBlockCountByTag(MemoryBlockTag tag) const {
        return 0;
      }

      /**
       * @brief Sets the out-of-memory handler called before the allocator
       *        panics.
       * @param handler Pointer to a concrete implementation of
       *                @ref IOutOfMemoryHandler, or `nullptr` to clear.
       */
      virtual void SetOutOfMemoryHandler(
        IOutOfMemoryHandler* handler
      ) {}

      /**
       * @brief Releases memory reserved for the initial image, returning those
       *        blocks to the free pool.
       *
       * After this call, the initial image blocks are available for normal
       * allocation. Calling this more than once is safe (subsequent calls
       * are no-ops).
       */
      virtual void FreeInitialImage() {}
  };
}
