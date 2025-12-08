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
       */
      static void Initialize();

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
  };
}
