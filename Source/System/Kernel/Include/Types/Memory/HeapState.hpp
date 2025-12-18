/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Memory/HeapState.hpp
 * Snapshot of current heap state.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Types::Memory {
  /**
   * Snapshot of current heap state.
   */
  struct HeapState {
    /**
     * Total heap bytes currently mapped.
     */
    UInt32 MappedBytes;

    /**
     * Total free bytes tracked by the heap.
     */
    UInt32 FreeBytes;

    /**
     * Number of free blocks in the heap.
     */
    UInt32 FreeBlocks;
  };
}
