/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Memory.hpp
 * Architecture-agnostic memory manager interface.
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
       * @return
       *   Pointer to the allocated page (identity mapped).
       */
      static void* AllocatePage(bool zero = false);

      /**
       * Returns the physical address of the kernel page directory.
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
       */
      static UInt32 CreateAddressSpace();

      /**
       * Destroys an address space created with CreateAddressSpace.
       */
      static void DestroyAddressSpace(UInt32 pageDirectoryPhysical);

      /**
       * Maps a virtual page in the specified address space.
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
       * Runs a simple test of page and heap allocation/free paths.
       * Only invoked when enabled by build flag.
       */
      static void Test();

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
  };
}
