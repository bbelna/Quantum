//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/BootInfo.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Bootloader-provided memory map and metadata passed into the kernel.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>
#include <Types/Memory/MemoryRegion.hpp>

namespace Quantum::Kernel::Types::Boot {
  using MemoryRegion = Types::Memory::MemoryRegion;

  /**
   * Bootloader-provided memory map and metadata passed into the kernel.
   */
  struct BootInfo {
    /**
     * Number of valid entries in the table.
     */
    UInt32 EntryCount;

    /**
     * Reserved for future use/alignment.
     */
    UInt32 Reserved;

    /**
     * E820 entries captured during boot.
     */
    MemoryRegion Entries[32];
  };
}
