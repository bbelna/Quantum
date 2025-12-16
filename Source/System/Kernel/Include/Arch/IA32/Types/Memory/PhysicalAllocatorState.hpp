/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/Memory/PhysicalAllocatorState.hpp
 * State of the physical allocator.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::Memory {
  /**
   * State of the physical allocator.
   */
  struct PhysicalAllocatorState {
    /**
     * Total pages managed by the allocator.
     */
    UInt32 TotalPages;

    /**
     * Pages currently marked used.
     */
    UInt32 UsedPages;

    /**
     * Pages currently available.
     */
    UInt32 FreePages;
  };
}
