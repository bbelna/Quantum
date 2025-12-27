/**
 * @file System/Kernel/Include/Memory.hpp
 * @brief Kernel memory management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  class Memory {
    public:
      /**
       * Snapshot of current heap state.
       */
      struct HeapState {
        /**
         * Total heap bytes currently mapped.
         */
        UInt32 mappedBytes;

        /**
         * Total free bytes tracked by the heap.
         */
        UInt32 freeBytes;

        /**
         * Number of free blocks in the heap.
         */
        UInt32 freeBlocks;
      };

      /**
       * Represents a free block of memory.
       */
      struct FreeBlock {
        /**
         * Size of the free block in bytes.
         */
        UInt32 size;

        /**
         * Pointer to the next free block in the linked list.
         */
        FreeBlock* next;
      };

      /**
       * Metadata stored immediately before an aligned payload.
       */
      struct AlignedMetadata {
        /**
         * Alignment marker to detect metadata.
         */
        UInt32 magic;

        /**
         * Owning free-block header for the allocation.
         */
        FreeBlock* block;

        /**
         * Offset from the start of the block payload to the aligned address.
         */
        UInt32 payloadOffset;
      };

      /**
       * Initializes the kernel memory subsystem (paging + allocators).
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot information block.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Allocates one 4 KB page of physical memory.
       * @param zero
       *   Whether to zero the page contents.
       * @return
       *   Pointer to the allocated page (identity mapped).
       */
      static void* AllocatePage(bool zero = false);

      /**
       * Returns the physical address of the kernel page directory.
       * @return
       *   Physical address of the kernel page directory.
       */
      static UInt32 GetKernelPageDirectoryPhysical();

      /**
       * Maps a virtual page to a physical page.
       * @param virtualAddress
       *   Virtual address of the page to map.
       * @param physicalAddress
       *   Physical address of the page to map.
       * @param writable
       *   Whether the page should be writable.
       * @param user
       *   Whether the page should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapPage(
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Creates a new address space and returns its page directory physical.
       * @return
       *   Physical address of the created page directory.
       */
      static UInt32 CreateAddressSpace();

      /**
       * Destroys an address space created with `CreateAddressSpace`.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to destroy.
       */
      static void DestroyAddressSpace(UInt32 pageDirectoryPhysical);

      /**
       * Maps a virtual page in the specified address space.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory.
       * @param virtualAddress
       *   Virtual address of the page to map.
       * @param physicalAddress
       *   Physical address of the page to map.
       * @param writable
       *   Whether the page should be writable.
       * @param user
       *   Whether the page should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapPageInAddressSpace(
        UInt32 pageDirectoryPhysical,
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Activates the given address space for the current CPU.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to activate.
       */
      static void ActivateAddressSpace(UInt32 pageDirectoryPhysical);

      /**
       * Allocates a block of kernel heap memory.
       * @param size
       *   Number of bytes requested.
       * @return
       *   Pointer to writable memory (never null; may panic on OOM).
       */
      static void* Allocate(Size size);

      /**
       * Allocates a block of kernel heap memory with a specific alignment.
       * @param size
       *   Number of bytes requested.
       * @param alignment
       *   Power-of-two byte alignment.
       * @return
       *   Pointer to aligned memory (never null; may panic on OOM).
       */
      static void* AllocateAligned(Size size, Size alignment);

      /**
       * Frees a single physical page (identity-mapped).
       * @param page
       *   Pointer to the page to free.
       */
      static void FreePage(void* page);

      /**
       * Frees a heap allocation previously returned by Allocate.
       * @param pointer
       *   Pointer to memory to free.
       */
      static void Free(void* pointer);

      /**
       * Retrieves the current heap state.
       * @return
       *   Snapshot of current heap state.
       */
      static HeapState GetHeapState();

      /**
       * Prints the current heap state to the console.
       */
      static void DumpState();

      /**
       * Verifies heap invariants (free list ordering, sizes, canaries).
       * Intended for debug builds; may panic on corruption.
       * @return
       *   True if no inconsistencies were detected; false otherwise.
       */
      static bool VerifyHeap();

      /**
       * Resets the heap allocator state (debug/boot use only).
       * Rebuilds a fresh free list over the currently mapped heap pages.
       */
      static void ResetHeap();

    private:
      /**
       * Heap page size.
       */
      static constexpr UInt32 _heapPageSize = 4096;

      /**
       * Heap start virtual address.
       */
      static UInt32 _heapStartVirtualAddress;

      /**
       * Heap region size in bytes.
       */
      static UInt32 _heapRegionBytes;

      /**
       * Number of guard pages before the heap.
       */
      static constexpr UInt32 _heapGuardPagesBefore = 1;

      /**
       * Number of guard pages after the heap.
       */
      static constexpr UInt32 _heapGuardPagesAfter = 1;

      /**
       * Magic tag placed before aligned allocations.
       */
      static constexpr UInt32 _alignedMagic = 0xA11A0CED;

      /**
       * Number of fixed-size bins.
       */
      static constexpr UInt32 _binCount = 4;

      /**
       * Poison pattern used to fill newly allocated payloads.
       */
      static constexpr UInt8 _poisonAllocated = 0xAA;

      /**
       * Poison pattern used to fill freed payloads.
       */
      static constexpr UInt8 _poisonFreed = 0x55;

      /**
       * Canary value stored at the end of each allocation.
       */
      static constexpr UInt32 _canaryValue = 0xDEADC0DE;

      /**
       * Sizes of fixed-size bins.
       */
      inline static const UInt32 _binSizes[_binCount] = { 16, 32, 64, 128 };

      /**
       * Pointer to the start of the heap region.
       */
      inline static UInt8* _heapBase = nullptr;

      /**
       * Pointer to the end of the mapped heap region (next unmapped byte).
       */
      inline static UInt8* _heapMappedEnd = nullptr;

      /**
       * Address of the guard page immediately following the mapped heap.
       */
      inline static UInt8* _guardAddress = nullptr;

      /**
       * Number of bytes currently mapped in the heap.
       */
      inline static UInt32 _heapMappedBytes = 0;

      /**
       * Pointer to the current position in the heap for allocations.
       */
      inline static UInt8* _heapCurrent = nullptr;

      /**
       * Tracks the minimum contiguous pages we should keep free at the tail of
       * the heap to satisfy the largest allocation request seen so far.
       */
      inline static UInt32 _requiredTailPages = 2;

      /**
       * Head of the general free list.
       */
      inline static FreeBlock* _freeList = nullptr;

      /**
       * Free lists for each fixed-size bin.
       */
      inline static FreeBlock* _binFreeLists[_binCount]
        = { nullptr, nullptr, nullptr, nullptr };

      /**
       * Writes the canary for a free block at the end of its payload.
       * @param block
       *   Block to write canary for.
       */
      static void SetFreeBlockCanary(FreeBlock* block);

      /**
       * Maps the next page in the heap virtual range, keeping a guard page
       * unmapped immediately after the mapped region.
       * @return
       *   Pointer to the start of the mapped page.
       */
      static UInt8* MapNextHeapPage();

      /**
       * Lazily initializes heap bookkeeping on first use.
       */
      static void EnsureHeapInitialized();

      /**
       * Merges adjacent free blocks to reduce fragmentation.
       */
      static void CoalesceAdjacentFreeBlocks();

      /**
       * Reclaims pages from the end of the heap so the mapped region remains
       * contiguous.
       */
      static void ReclaimPageSpans();

      /**
       * Inserts a free block into the sorted free list and coalesces neighbors.
       * @param block
       *   Block to insert.
       */
      static void InsertFreeBlockSorted(FreeBlock* block);

      /**
       * Attempts to satisfy an allocation from the general free list.
       * @param needed
       *   Total bytes requested including header.
       * @return
       *   Pointer to payload or `nullptr` if none fit.
       */
      static void* AllocateFromFreeList(UInt32 needed);

      /**
       * Determines the bin index for a requested payload size.
       * @param size
       *   Payload bytes requested.
       * @return
       *   Bin index or -1 if it does not fit in a fixed bin.
       */
      static int BinIndexForSize(UInt32 size);

      /**
       * Derives the original payload size (without canary/padding) from a
       * stored block size.
       * @param blockSize
       *   Size stored in the block header (payload + canary + padding).
       * @return
       *   Payload size rounded down to the allocator's 8-byte alignment.
       */
      static UInt32 PayloadSizeFromBlock(UInt32 blockSize);

      /**
       * Allocates from a fixed-size bin if available, otherwise falls back to
       * free list.
       * @param binSize
       *   Bin payload size to request.
       * @param neededWithHeader
       *   Total bytes including header.
       * @return
       *   Pointer to payload or nullptr.
       */
      static void* AllocateFromBin(UInt32 binSize, UInt32 neededWithHeader);

      /**
       * Returns a freed block either to a size bin or the general free list.
       * @param block
       *   Block being freed.
       */
      static void InsertIntoBinOrFreeList(FreeBlock* block);
  };
}
