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
    UInt32 baseLow;     /**< Low 32 bits of the physical base address. */
    UInt32 baseHigh;    /**< High 32 bits of the physical base address. */
    UInt32 lengthLow;   /**< Low 32 bits of the segment length in bytes. */
    UInt32 lengthHigh;  /**< High 32 bits of the segment length in bytes. */
    UInt32 type;        /**< Region classification (1 = usable, otherwise reserved). */
  };

  /**
   * Bootloader-provided memory map and metadata passed into the kernel.
   */
  struct BootInfo {
    UInt32 entryCount;          /**< Number of valid entries in the table. */
    UInt32 reserved;            /**< Reserved for future use/alignment. */
    MemoryRegion entries[32];   /**< E820 entries captured during boot. */
  };

  constexpr UInt32 kBootInfoPhysical = 0x00008000;
}
