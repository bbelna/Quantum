//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/BootInfo.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Boot-time information passed from the bootloader to the kernel.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel {
  /**
   * Describes a single physical memory segment reported by BIOS E820.
   * type == 1 indicates usable RAM; other values are reserved or ACPI data.
   */
  struct MemoryRegion {
    uint32 baseLow;     /**< Low 32 bits of the physical base address. */
    uint32 baseHigh;    /**< High 32 bits of the physical base address. */
    uint32 lengthLow;   /**< Low 32 bits of the segment length in bytes. */
    uint32 lengthHigh;  /**< High 32 bits of the segment length in bytes. */
    uint32 type;        /**< Region classification (1 = usable, otherwise reserved). */
  };

  /**
   * Bootloader-provided memory map and metadata passed into the kernel.
   */
  struct BootInfo {
    uint32 entryCount;          /**< Number of valid entries in the table. */
    uint32 reserved;            /**< Reserved for future use/alignment. */
    MemoryRegion entries[32];   /**< E820 entries captured during boot. */
  };

  constexpr uint32 kBootInfoPhysical = 0x00008000;
}
