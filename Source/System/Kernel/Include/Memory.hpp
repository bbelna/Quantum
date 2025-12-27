/**
 * @file System/Kernel/Include/Memory.hpp
 * @brief Kernel memory management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  class Memory {
    public:
      /**
       * Initializes the kernel memory subsystem (paging + allocators).
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot information block.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Allocates one 4 KB page of physical memory.
       * @param zero
       *   Whether to zero the page contents.
       * @return
       *   Pointer to the allocated page (identity mapped).
       */
      static void* AllocatePage(bool zero = false);

      /**
       * Returns the physical address of the kernel page directory.
       * @return
       *   Physical address of the kernel page directory.
       */
      static UInt32 GetKernelPageDirectoryPhysical();

      /**
       * Maps a virtual page to a physical page.
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
       * Creates a new address space and returns its page directory physical.
       * @return
       *   Physical address of the created page directory.
       */
      static UInt32 CreateAddressSpace();

      /**
       * Destroys an address space created with `CreateAddressSpace`.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to destroy.
       */
      static void DestroyAddressSpace(UInt32 pageDirectoryPhysical);

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
      static void ActivateAddressSpace(UInt32 pageDirectoryPhysical);

      /**
       * Frees a single physical page (identity-mapped).
       * @param page
       *   Pointer to the page to free.
       */
      static void FreePage(void* page);
  };
}
