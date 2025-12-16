/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Memory/MemoryRegion.hpp
 * Describes a single physical memory segment reported by BIOS E820.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::Kernel::Types::Memory {
  /**
   * Describes a single physical memory segment reported by BIOS E820.
   */
  struct MemoryRegion {
    /**
     * Low 32 bits of the physical base address.
     */
    UInt32 BaseLow;

    /**
     * High 32 bits of the physical base address.
     */
    UInt32 BaseHigh;

    /**
     * Low 32 bits of the segment length in bytes.
     */
    UInt32 LengthLow;

    /**
     * High 32 bits of the segment length in bytes.
     */
    UInt32 LengthHigh;

    /**
     * Region classification (1 = usable, otherwise reserved).
     */
    UInt32 Type;
  };
}