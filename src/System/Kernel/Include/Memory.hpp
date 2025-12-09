//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Memory.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Architecture-agnostic memory manager interface.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel {
  class Memory {
    public:
      /**
       * Initializes the kernel memory subsystem (paging + allocators).
       * @param bootInfoPhysicalAddress Physical address of the boot information
       * block.
       */
      static void Initialize(uint32 bootInfoPhysicalAddress);

      /**
       * Allocates one 4 KB page of physical memory.
       * @return Pointer to the allocated page (identity mapped).
       */
      static void* AllocatePage();

      /**
       * Allocates a block of kernel heap memory (bump allocator).
       * @param size Number of bytes requested.
       * @return Pointer to writable memory (never null; may panic on OOM).
       */
      static void* Allocate(usize size);

      /**
       * Frees a single physical page (identity-mapped).
       * @param page Pointer to the page to free.
       */
      static void FreePage(void* page);

      /**
       * Frees a heap allocation previously returned by Allocate.
       * @param pointer Pointer to memory to free.
       */
      static void Free(void* pointer);

      /**
       * Snapshot of current heap state.
       */
      struct HeapState {
        /**
         * Total heap bytes currently mapped.
         */
        uint32 mappedBytes;

        /**
         * Total free bytes tracked by the heap.
         */
        uint32 freeBytes;

        /**
         * Number of free blocks in the heap.
         */
        uint32 freeBlocks;
      };

      /**
       * Retrieves the current heap state.
       * @return Snapshot of current heap state.
       */
      static HeapState GetHeapState();
  };
}
