//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/BootInfo.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Boot-time information passed from the bootloader to the kernel.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel {
  /**
   * Describes a single physical memory segment reported by BIOS E820.
   * type == 1 indicates usable RAM; other values are reserved or ACPI data.
   */
  struct MemoryRegion {
    /**
     * Low 32 bits of the physical base address.
     */
    UInt32 baseLow;

    /**
     * High 32 bits of the physical base address.
     */
    UInt32 baseHigh;

    /**
     * Low 32 bits of the segment length in bytes.
     */
    UInt32 lengthLow;

    /**
     * High 32 bits of the segment length in bytes.
     */
    UInt32 lengthHigh;

    /**
     * Region classification (1 = usable, otherwise reserved).
     */
    UInt32 type;
  };

  /**
   * Bootloader-provided memory map and metadata passed into the kernel.
   */
  struct BootInfo {
    /**
     * Number of valid entries in the table.
     */
    UInt32 entryCount;

    /**
     * Reserved for future use/alignment.
     */
    UInt32 reserved;

    /**
     * E820 entries captured during boot.
     */
    MemoryRegion entries[32];
  };

  constexpr UInt32 kBootInfoPhysical = 0x00008000;
}
