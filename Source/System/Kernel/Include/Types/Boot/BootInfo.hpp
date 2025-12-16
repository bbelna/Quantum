/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Boot/BootInfo.hpp
 * Bootloader-provided memory map and metadata passed into the kernel.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/Memory/MemoryRegion.hpp>

namespace Quantum::System::Kernel::Types::Boot {
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
     * Firmware memory map entries.
     */
    MemoryRegion Entries[32];
  };
}
