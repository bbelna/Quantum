/**
 * @file System/Kernel/Include/Arch/IA32/BootInfo.hpp
 * @brief IA32 boot info handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 boot info handling.
   */
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

      /**
       * Maximum number of boot info entries.
       */
      static constexpr UInt32 maxEntries = 32;

      /**
       * Size of the raw boot info structure.
       */
      static constexpr UInt32 rawSize = sizeof(Raw);

      /**
       * Cached view of the boot info data.
       */
      struct View {
        UInt32 entryCount;
        UInt32 reserved;
        UInt32 initBundlePhysical;
        UInt32 initBundleSize;
        Region entries[maxEntries];
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

    private:
      /**
       * Cached boot info view for IA32.
       */
      [[gnu::section(".text.start.data")]]
      inline static View _bootInfoView {};

      /**
       * Physical address of the boot info structure.
       */
      [[gnu::section(".text.start.data")]]
      inline static UInt32 _bootInfoPhysicalAddress = 0;

      /**
       * Whether the cached boot info view is valid.
       */
      [[gnu::section(".text.start.data")]]
      inline static UInt32 _bootInfoValid = 0;
  };
}
