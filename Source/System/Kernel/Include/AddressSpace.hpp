/**
 * @file System/Kernel/Include/AddressSpace.hpp
 * @brief Address space and page mapping helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * Address space and page mapping helpers.
   */
  class AddressSpace {
    public:
      /**
       * Returns the physical address of the kernel page directory.
       * @return
       *   Physical address of the kernel page directory.
       */
      static UInt32 GetKernelPageDirectoryPhysical();

      /**
       * Maps a virtual page to a physical page in the current address space.
       * @param virtualAddress
       *   Virtual address of the page to map.
       * @param physicalAddress
       *   Physical address of the page to map.
       * @param writable
       *   Whether the page should be writable.
       * @param user
       *   Whether the page should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapPage(
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Creates a new address space.
       * @return
       *   Physical address of the created page directory.
       */
      static UInt32 Create();

      /**
       * Destroys an address space created with Create.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to destroy.
       */
      static void Destroy(UInt32 pageDirectoryPhysical);

      /**
       * Maps a virtual page in the specified address space.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory.
       * @param virtualAddress
       *   Virtual address of the page to map.
       * @param physicalAddress
       *   Physical address of the page to map.
       * @param writable
       *   Whether the page should be writable.
       * @param user
       *   Whether the page should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapPageInAddressSpace(
        UInt32 pageDirectoryPhysical,
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Activates the given address space for the current CPU.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to activate.
       */
      static void Activate(UInt32 pageDirectoryPhysical);
  };
}
