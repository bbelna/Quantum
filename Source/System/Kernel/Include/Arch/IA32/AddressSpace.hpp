/**
 * @file System/Kernel/Include/Arch/IA32/AddressSpace.hpp
 * @brief IA32 address space management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 address space helpers.
   */
  class AddressSpace {
    public:
      /**
       * Creates a new address space and returns its page directory physical.
       * @return
       *   Physical address of the new page directory.
       */
      static UInt32 Create();

      /**
       * Destroys an address space created by Create.
       * @param pageDirectoryPhysicalAddress
       *   Physical address of the page directory to destroy.
       */
      static void Destroy(UInt32 pageDirectoryPhysicalAddress);

      /**
       * Maps a virtual page in the specified address space.
       * @param pageDirectoryPhysicalAddress
       *   Physical address of the target page directory.
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
        UInt32 pageDirectoryPhysicalAddress,
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Activates the specified address space.
       * @param pageDirectoryPhysicalAddress
       *   Physical address of the page directory to activate.
       */
      static void Activate(UInt32 pageDirectoryPhysicalAddress);
  };
}
