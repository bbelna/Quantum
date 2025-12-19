/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/BootInfo.hpp
 * IA32 boot info parsing and caching.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  class BootInfo {
    public:
      /**
       * Describes a single physical memory segment reported by BIOS E820.
       */
      struct Region {
        UInt32 baseLow;
        UInt32 baseHigh;
        UInt32 lengthLow;
        UInt32 lengthHigh;
        UInt32 type;
      };

      /**
       * Raw boot info layout provided by the bootloader.
       */
      struct Raw {
        UInt32 entryCount;
        UInt32 reserved;
        UInt32 initBundlePhysical;
        UInt32 initBundleSize;
        Region entries[32];
      };

      static constexpr UInt32 MaxEntries = 32;
      static constexpr UInt32 RawSize = sizeof(Raw);

      /**
       * Cached view of the boot info data.
       */
      struct View {
        UInt32 entryCount;
        UInt32 reserved;
        UInt32 initBundlePhysical;
        UInt32 initBundleSize;
        Region entries[MaxEntries];
      };

      /**
       * Initializes the cached boot info view from a physical address.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Returns the cached boot info view, or nullptr if unavailable.
       */
      static const View* Get();

      /**
       * Returns the physical address supplied by the bootloader.
       */
      static UInt32 GetPhysicalAddress();
  };
}
