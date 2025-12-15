//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Types/MemoryRegion.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Describes a single physical memory segment reported by BIOS E820.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel::Types::Memory {
  /**
   * Describes a single physical memory segment reported by BIOS E820.
   * type == 1 indicates usable RAM; other values are reserved or ACPI data.
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