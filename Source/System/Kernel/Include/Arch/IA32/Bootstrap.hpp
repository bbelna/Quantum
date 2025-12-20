/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Bootstrap.hpp
 * IA32 kernel bootstrap helpers.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 kernel bootstrap helpers.
   */
  class Bootstrap {
    public:
      /**
       * Captures boot info from the bootloader for early logging.
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot info structure.
       */
      static void CaptureBootInfo(UInt32 bootInfoPhysicalAddress);

      /**
       * Builds identity and higher-half mappings needed to turn on paging.
       */
      static void BuildBootstrapPaging();

      /**
       * Returns the physical address of the bootstrap page directory.
       * @return
       *   Physical address of the bootstrap page directory.
       */
      static UInt32 GetBootstrapPageDirectoryPhysical();

      /**
       * Writes cached boot info details to the log.
       */
      static void TraceBootInfo();

    private:
      /**
       * Bootstrap page directory used before the main memory manager takes
       * over.
       */
      alignas(4096) static UInt32 _bootstrapPageDirectory[1024];

      /**
       * Page tables covering the 16 MB identity window.
       */
      alignas(4096) static UInt32 _bootstrapPageTables[4][1024];

      /**
       * Page tables for the higher-half kernel image during bootstrap.
       */
      alignas(4096) static UInt32 _bootstrapKernelTables[8][1024];

      /**
       * Present page flag.
       */
      static constexpr UInt32 _pagePresent = 0x1;

      /**
       * Writable page flag.
       */
      static constexpr UInt32 _pageWrite = 0x2;

      /**
       * Recursive page table slot.
       */
      static constexpr UInt32 _recursiveSlot = 1023;

      /**
       * IA32 page size in bytes.
       */
      static constexpr UInt32 _pageSize = 4096;

      /**
       * Size of the identity-mapped window during bootstrap.
       */
      static constexpr UInt32 _identityWindowBytes = 16 * 1024 * 1024;

      /**
       * Physical address of the init bundle.
       */
      static UInt32 _bootInitBundlePhys;

      /**
       * Size of the init bundle.
       */
      static UInt32 _bootInitBundleSize;

      /**
       * Magic value 0 from the init bundle header.
       */
      static UInt32 _bootInitBundleMagic0;

      /**
       * Magic value 1 from the init bundle header.
       */
      static UInt32 _bootInitBundleMagic1;

      /**
       * Physical address of the boot info structure.
       */
      static UInt32 _bootInfoPhysical;

      /**
       * Number of entries in the boot info structure.
       */
      static UInt32 _bootInfoEntryCount;

      /**
       * Reserved field from the boot info structure.
       */
      static UInt32 _bootInfoReserved;
  };
}
