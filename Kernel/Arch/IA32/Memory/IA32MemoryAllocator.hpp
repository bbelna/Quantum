/**
 * @file Kernel/Arch/IA32/Memory/IA32MemoryAllocator.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::IA32MemoryAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Handlers/IMaydayHandler.hpp>
#include <KernelTypes.hpp>
#include <Memory/BuddyAllocator.hpp>
#include <Memory/BuddyFreeNode.hpp>
#include <Memory/IMemoryAllocator.hpp>

#include "E820BootInfo.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 implementation of
   *        @ref @QKrnl::Memory::IMemoryAllocator.
   *
   * Manages physical memory in block-sized units using a buddy
   * allocator. Supports both bottom-up (early boot, DMA-friendly) and
   * top-down (normal runtime) allocation strategies. Maintains per-block
   * reference counts so that shared pages (e.g., CoW after fork) are
   * only freed when the last reference is released. Regions occupied by
   * the kernel image, the metadata arrays, and the initial image are
   * reserved at initialization and excluded from the free pool.
   */
  class IA32MemoryAllocator : public IMemoryAllocator {
    public:
      /**
       * @brief Initializes the @ref IA32MemoryAllocator.
       * @param maydayHandler Pointer to a concrete implementation of
       *                      @ref IMaydayHandler.
       * @param bootInfo Pointer to the @ref E820BootInfo.
       */
      void Initialize(
        IMaydayHandler* maydayHandler,
        E820BootInfo* bootInfo
      );

      /**
       * @brief Enables top-down allocation.
       *
       * Must only be called after the kernel page directory is loaded and
       * identity mapping covers all kernel memory.
       */
      void EnableTopDownAllocation();

      /**
       * @brief Allocates a @ref MemoryBlock.
       * @return The newly allocated @ref MemoryBlock, or an invalid
       *         @ref MemoryBlock if allocation fails.
       */
      MemoryBlock Allocate() override;

      /**
       * @brief Allocates a @ref MemoryBlock of the given @p size.
       * @param size The size of the block to allocate.
       * @return The newly allocated @ref MemoryBlock, or an
       *         invalid @ref MemoryBlock if allocation fails.
       */
      MemoryBlock Allocate(Size size) override;

      /**
       * @brief Allocates a @ref MemoryBlock whose address range is below the
       *        given limit.
       * @param limit The exclusive upper bound on the address block.
       * @return The newly allocated @ref MemoryBlock, or an invalid
       *         @ref MemoryBlock if none is available below the limit.
       */
      MemoryBlock AllocateBelow(UInt32 limit) override;

      /**
       * @brief Frees a @ref MemoryBlock.
       * @param block The @ref MemoryBlock to free.
       */
      void Free(MemoryBlock block) override;

      /**
       * @brief Increments the reference count on a @ref MemoryBlock.
       * @param block The @ref MemoryBlock to retain.
       */
      void Retain(MemoryBlock block) override;

      /**
       * @brief Reserves a @ref MemoryBlock so it will not be allocated.
       * @param block The @ref MemoryBlock to reserve.
       */
      void Reserve(MemoryBlock block);

      /**
       * @brief Releases a previously reserved @ref MemoryBlock.
       * @param block The @ref MemoryBlock to release.
       */
      void Release(MemoryBlock block);

      /**
       * @brief Gets the total bytes under management by the allocator.
       * @return The total bytes under management by the allocator.
       */
      inline Size GetManagedBytes() const override {
        return _managedBytes;
      }

      /**
       * @brief Gets the size of blocks allocated by this allocator.
       * @return The block size in bytes.
       */
      inline Size GetBlockSize() const override {
        return _blockSize;
      }

      /**
       * @brief Gets the total number of blocks under management.
       * @return The total block count.
       */
      inline Size GetBlockCount() const override {
        return _blockCount;
      }

      /**
       * @brief Gets the number of used (allocated) blocks.
       * @return The used block count.
       */
      inline Size GetUsedBlockCount() const override {
        return _usedBlockCount;
      }

      /**
       * @brief Gets the total bytes reserved at boot (kernel image, bitmap,
       *        ref-count array, initial image).
       * @return The reserved byte count.
       */
      inline Size GetReservedBytes() const override {
        return _reservedBytes;
      }

      /**
       * @brief Gets the bytes currently reserved for the initial image
       *        (startup bundle). Returns 0 after @ref FreeInitialImage.
       * @return The initial image byte count.
       */
      inline Size GetInitialImageBytes() const override {
        return _initialImageReservedBytes;
      }

      /**
       * @brief Allocates a single @ref MemoryBlock tagged with the given
       *        @ref MemoryBlockTag.
       * @param tag The @ref MemoryBlockTag to associate.
       * @return The allocated @ref MemoryBlock, or an invalid
       *         @ref MemoryBlock on failure.
       */
      MemoryBlock Allocate(MemoryBlockTag tag) override;

      /**
       * @brief Allocates a contiguous @ref MemoryBlock tagged with the given
       *        @ref MemoryBlockTag.
       * @param size The size in bytes (rounded up to block size).
       * @param tag The @ref MemoryBlockTag to associate.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      MemoryBlock Allocate(
        Size size,
        MemoryBlockTag tag
      ) override;

      /**
       * @brief Allocates a single @ref MemoryBlock below the given address
       *        limit, tagged with the given @ref MemoryBlockTag.
       * @param limit The exclusive upper bound on the kernel address.
       * @param tag The @ref MemoryBlockTag to associate.
       * @return The allocated @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         if none available.
       */
      MemoryBlock AllocateBelow(
        UInt32 limit,
        MemoryBlockTag tag
      ) override;

      /**
       * @brief Returns the number of used blocks for a given tag.
       * @param tag The @ref MemoryBlockTag to query.
       * @return The number of blocks allocated with this tag.
       */
      Size GetUsedBlockCountByTag(MemoryBlockTag tag) const override;

      /**
       * @brief Sets the out-of-memory handler called before the allocator
       *        panics.
       * @param handler Pointer to the OOM handler, or `nullptr` to clear.
       */
      void SetOutOfMemoryHandler(IOutOfMemoryHandler* handler) override;

      /**
       * @brief Releases the initial image (startup bundle) memory reserved
       *        at boot, returning those physical pages to the free pool.
       */
      void FreeInitialImage() override;

    private:
      /**
       * @brief Converts a physical address to a @ref BuddyFreeNode
       *        pointer via identity mapping.
       */
      static BuddyFreeNode<UInt32>* _toNode(UInt32 address) {
        return reinterpret_cast<
          BuddyFreeNode<UInt32>*
        >(address);
      }

      /**
       * @brief Pointer to a concrete implementation of
       *        @ref IMaydayHandler.
       */
      IMaydayHandler* _maydayHandler = nullptr;

      /**
       * @brief Maximum number of block frames supported.
       */
      constexpr static Size _maxBlockFrames = 64;

      /**
       * @brief Total bytes under management by the allocator.
       */
      Size _managedBytes = 0;

      /**
       * @brief Size of each block managed by the allocator.
       */
      Size _blockSize = 0;

      /**
       * @brief Total number of blocks managed by the allocator.
       */
      Size _blockCount = 0;

      /**
       * @brief Number of used blocks.
       */
      Size _usedBlockCount = 0;

      /**
       * @brief Bytes reserved at boot time via @ref Reserve.
       */
      Size _reservedBytes = 0;

      /**
       * @brief Bytes reserved for the initial image (startup bundle).
       *
       * Set during initialization. Cleared to `0` by @ref FreeInitialImage.
       */
      Size _initialImageReservedBytes = 0;

      /**
       * @brief When `true`, @ref Allocate scans top-down (high memory first).
       *        When `false`, scans bottom-up (low memory first).
       */
      bool _topDownEnabled = false;

      /**
       * @brief @ref MemoryBlock interval occupied by the initial image.
       */
      Interval<UInt32> _initialImageInterval;

      /**
       * @brief The architecture-independent buddy allocator.
       */
      BuddyAllocator<UInt32> _buddy;

      /**
       * @brief Per-frame reference counts.
       *
       * Indexed by block index (`kernelAddress / blockSize`). A count of 0
       * means untracked (reserved or never allocated). A count >= 1 means
       * actively allocated and reference-tracked.
       */
      UInt32* _refCounts = nullptr;

      /**
       * @brief Array of block frames.
       */
      Interval<UInt32> _blockFrames[_maxBlockFrames];

      /**
       * @brief Number of block frames in the @ref _blockFrames array.
       */
      Size _blockFrameCount = 0;

      /**
       * @brief Per-block allocation tag. Indexed by block index.
       */
      MemoryBlockTag* _blockTags = nullptr;

      /**
       * @brief Per-tag used block counts, indexed by
       *        `static_cast<UInt8>(MemoryBlockTag)`.
       */
      Size _tagCounts[static_cast<UInt8>(MemoryBlockTag::Count)] = {};

      /**
       * @brief Optional out-of-memory handler invoked before `MAYDAY`.
       */
      IOutOfMemoryHandler* _oomHandler = nullptr;

      /**
       * @brief Calculates the total memory size from the boot info.
       * @param bootInfo Pointer to the @ref E820BootInfo.
       * @return Total memory size in bytes.
       */
      Size _calculateManagedBytes(E820BootInfo* bootInfo);

      /**
       * @brief Calculates the block interval occupied by the initial image.
       * @param bootInfo Pointer to the @ref E820BootInfo.
       * @return The block interval of the initial image.
       */
      Interval<UInt32> _calculateInitialImageInterval(E820BootInfo* bootInfo);

      /**
       * @brief Initializes the buddy allocator metadata arrays and builds
       *        the buddy free lists from usable E820 regions.
       * @param bootInfo Pointer to the @ref E820BootInfo.
       */
      void _initializeAllocator(E820BootInfo* bootInfo);

      /**
       * @brief Zeros out the memory in the given block.
       * @param block The block to zero.
       */
      void _zero(MemoryBlock block);
  };
}
