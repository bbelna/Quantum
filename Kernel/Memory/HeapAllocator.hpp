/**
 * @file Kernel/Memory/HeapAllocator.hpp
 * @brief Declares @ref @QKrnl::Memory::HeapAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "AddressSpaceMap.hpp"
#include "AlignedHeapAllocationHeader.hpp"
#include "FreeHeapBlock.hpp"
#include "HeapAllocatorConfiguration.hpp"
#include "HeapBlock.hpp"
#include "IMemoryAllocator.hpp"
#include "IMemoryMapper.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Allocates memory for dynamic kernel objects and data structures.
   *
   * The heap allocator manages a contiguous virtual address range that is
   * backed by kernel memory blocks mapped on demand. It maintains fixed-size
   * bins for small allocations and a sorted general free list for larger
   * requests. Freed blocks are coalesced to reduce fragmentation and
   * returned to bins when possible.
   *
   * Guard blocks are placed at the boundaries of the mapped heap region to
   * detect out-of-bounds accesses. Poison bytes and canary values provide
   * runtime diagnostics for use-after-free and buffer-overflow bugs.
   */
  class HeapAllocator {
    public:
      /**
       * @brief Number of fixed-size bins to maintain for small allocations.
       */
      static constexpr Size BinCount = 6;

      /**
       * @brief The minimum number of contiguous free blocks to keep at the tail
       *        of the heap.
       *
       * This serves as a floor for @ref HeapAllocatorConfiguration
       * @ref HeapAllocatorConfiguration::RequiredTailBlocks to ensure we can
       * always satisfy the largest allocation request seen so far, even if the
       * heap allocator is constructed with a smaller number of required tail
       * blocks.
       */
      static constexpr Size MinimumTailBlocks = 2;

      /**
       * @brief Initializes the @ref HeapAllocator.
       * @param memoryAllocator
       *   Pointer to a concrete implementation of @ref IMemoryAllocator.
       * @param memoryMapper
       *   Pointer to a concrete implementation of @ref IMemoryMapper.
       * @param maydayHandler
       *   Pointer to a concrete implementation of @ref IMaydayHandler.
       * @param pressure
       *   Pointer to the @ref MemoryPressureMonitor.
       * @param configuration
       *   @ref HeapAllocatorConfiguration struct with heap parameters.
       */
      void Initialize(
        IMemoryAllocator* memoryAllocator,
        IMemoryMapper* memoryMapper,
        IMaydayHandler* maydayHandler,
        MemoryPressureMonitor* pressure,
        HeapAllocatorConfiguration configuration
      );

      /**
       * @brief Allocates a block of heap memory of the given @p size.
       * @param size The size of the block to allocate.
       * @return Pointer to writable memory, or `nullptr` if the heap is
       *         exhausted and cannot be grown.
       *
       * Small allocations (up to 512 bytes) are served from fixed-size bins
       * for speed; larger allocations use the general free list or bump the
       * heap pointer and map new blocks on demand. The returned pointer is
       * aligned to 8 bytes. The caller is responsible for freeing the memory
       * via @ref Free.
       */
      void* Allocate(Size size);

      /**
       * @brief Frees a block of heap memory previously obtained from
       *        @ref Allocate.
       * @param pointer The pointer to the memory to free. Passing `nullptr`
       *                is a no-op.
       *
       * The freed block is returned to a fixed-size bin if it fits, or
       * inserted into the sorted general free list and coalesced with
       * adjacent free blocks to reduce fragmentation.
       */
      void Free(void* pointer);

      /**
       * @brief Returns the number of bytes currently mapped for the heap.
       * @return The mapped heap size in bytes.
       */
      Size GetMappedBytes() const {
        return _mappedBytes;
      }

    private:
      /**
       * @brief Pointer to a concrete implementation of @ref IMemoryAllocator.
       */
      IMemoryAllocator* _memoryAllocator = nullptr;

      /**
       * @brief Pointer to a concrete implementation of @ref IMemoryMapper.
       */
      IMemoryMapper* _memoryMapper = nullptr;

      /**
       * @brief Pointer to a concrete implementation of @ref IMaydayHandler.
       */
      IMaydayHandler* _maydayHandler = nullptr;

      /**
       * @brief Pointer to the @ref MemoryPressureMonitor.
       */
      MemoryPressureMonitor* _pressure = nullptr;

      /**
       * @brief Pointer to the @ref IAddressSpace the heap is allocated in.
       */
      IAddressSpace* _addressSpace = nullptr;

      /**
       * @brief The @ref MemoryBlock describing the heap region to manage.
       */
      MemoryBlock _block;

      /**
       * @brief Number of guard blocks to place before the heap.
       */
      Size _guardBlockCountBefore;

      /**
       * @brief Number of guard blocks to place after the heap.
       */
      Size _guardBlockCountAfter;

      /**
       * @brief Sizes of fixed-size bins.
       */
      const Size _binSizes[BinCount] = {
        16,
        32,
        64,
        128,
        256,
        512
      };

      /**
       * @brief Pointer to the start of the heap region.
       */
      UInt8* _base = nullptr;

      /**
       * @brief Pointer to the end of the mapped heap region (next unmapped
       *        byte).
       */
      UInt8* _mappedEnd = nullptr;

      /**
       * @brief Address of the guard block immediately following the mapped heap.
       */
      UInt8* _guardAddress = nullptr;

      /**
       * @brief Number of bytes currently mapped in the heap.
       */
      Size _mappedBytes = 0;

      /**
       * @brief Pointer to the current position in the heap for allocations.
       */
      UInt8* _current = nullptr;

      /**
       * @brief
       *   Tracks the minimum contiguous blocks we should keep free at the tail
       *   of the heap to satisfy the largest allocation request seen so far.
       */
      Size _requiredTailBlocks;

      /**
       * @brief Pointer to the head of the @ref FreeHeapBlock list.
       */
      FreeHeapBlock* _freeList = nullptr;

      /**
       * @brief Free lists for each fixed-size bin.
       */
      FreeHeapBlock* _binFreeLists[BinCount] = {
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
      };

      /**
       * @brief The magic value to use for aligned allocation metadata.
       */
      UIntPtr _alignedMagic;

      /**
       * @brief The poison byte to write for allocated memory.
       */
      UInt8 _allocatedPoison;

      /**
       * @brief The poison byte to write for freed memory.
       */
      UInt8 _freedPoison;

      /**
       * @brief The canary value to write for free blocks.
       */
      UIntPtr _canary;

      /**
       * @brief The sentinel value to write for allocated blocks.
       */
      UIntPtr _allocatedSentinel;

      /**
       * @brief Maps the next block in the heap process range, keeping a guard
       *       block unmapped immediately after the mapped region.
       * @return @ref UInt8 pointer to the start of the mapped block.
       */
      UInt8* _mapNextBlock();

      /**
       * @brief Reclaims blocks from the end of the heap so the mapped region
       *        remains contiguous.
       */
      void _reclaimBlockSpans();

      /**
       * @brief Attempts to satisfy an allocation from the general free list.
       * @param needed Total bytes requested including header.
       * @return `void` pointer to payload or `nullptr` if none fit.
       */
      void* _allocateFromFreeList(Size needed);

      /**
       * @brief Allocates from a fixed-size bin if available, otherwise falls
       *        back to free list.
       * @param binSize Bin payload size to request.
       * @param neededWithHeader Total bytes including header.
       * @return `void` pointer to payload or `nullptr`.
       */
      void* _allocateFromBin(
        Size binSize,
        Size neededWithHeader
      );

      /**
       * @brief Writes the canary for a free block at the end of its payload.
       * @param block Pointer to the @ref FreeHeapBlock to write the canary for.
       */
      void _setFreeHeapBlockCanary(FreeHeapBlock* block);

      /**
       * @brief Inserts a free block into the sorted free list and coalesces
       *        neighbors.
       * @param block Pointer to the @ref FreeHeapBlock being freed.
       */
      void _insertFreeHeapBlockSorted(FreeHeapBlock* block);

      /**
       * @brief Returns a freed block either to a size bin or the general free
       *        list.
       * @param block Pointer to the @ref FreeHeapBlock being freed.
       */
      void _insertIntoBinOrFreeList(FreeHeapBlock* block);

      /**
       * @brief Merges adjacent @ref FreeHeapBlock to reduce fragmentation.
       */
      void _coalesceAdjacentFreeHeapBlocks();

      /**
       * @brief Determines the bin index for a requested payload size.
       * @param size Payload bytes requested.
       * @return Bin index or `-1` if it does not fit in a fixed bin.
       */
      int _binIndexForSize(Size size);

      /**
       * @brief
       *   Derives the original payload size (without canary/padding) from a
       *   stored block size.
       * @param blockSize
       *   Size stored in the block header (payload + canary + padding).
       * @return
       *   Payload size rounded down to the allocator's 8-byte alignment.
       */
      Size _payloadSizeFromBlock(Size blockSize);
  };
}
