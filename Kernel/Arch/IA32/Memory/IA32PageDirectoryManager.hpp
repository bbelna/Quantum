/**
 * @file Kernel/Arch/IA32/Memory/IA32PageDirectoryManager.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::PageDirectoryManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/IA32/Drivers/IA32DriverTypes.hpp>
#include <Memory/IAddressSpaceAllocator.hpp>

#include "IA32MemoryAllocator.hpp"
#include "IA32PageConstants.hpp"
#include "IA32PageDirectory.hpp"
#include "IA32PageTable.hpp"

/**
 * @brief The virtual address for the recursive page directory mapping.
 *
 * The last page directory entry (index 1023) points back to the page
 * directory itself. This causes the MMU to interpret the page directory
 * as a page table, making all page directory entries accessible at
 * 0xFFFFF000. Individual page tables are accessible at
 * 0xFFC00000 + (pdIndex * 0x1000).
 */
#define RECURSIVE_PAGE_DIRECTORY_VIRTUAL_ADDRESS 0xFFFFF000

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Manages @ref IA32PageDirectory.
   *
   * Responsible for allocating and freeing @ref IA32PageDirectory, ensuring
   * @ref IA32PageTable exist for a given @ref IA32PageDirectory index, and
   * cloning kernel-space mappings into newly created user address spaces.
   *
   * Uses the recursive @ref IA32PageDirectory mapping (PDE 1023 pointing to
   * itself) to access @ref IA32PageTable without identity-mapping them.
   */
  class IA32PageDirectoryManager : public IAddressSpaceAllocator {
    public:
      /**
       * @brief Initializes the @ref IA32PageDirectoryManager.
       * @param cpu Pointer to the @ref IA32CPUDriver.
       * @param memoryAllocator Pointer to the @ref IA32MemoryAllocator.
       */
      void Initialize(
        IA32CPUDriver* cpu,
        IA32MemoryAllocator* physicalBlockAllocator
      );

      /**
       * @brief Creates a new @ref IA32PageDirectory.
       * @return Pointer to the new @ref IA32PageDirectory.
       */
      IA32PageDirectory* Allocate() override;

      /**
       * @brief Frees a @ref IA32PageDirectory.
       * @param addressSpace Pointer to the @ref IAddressSpace to free.
       */
      void Free(IAddressSpace* addressSpace) override;

      /**
       * @brief
       *   Ensures that a @ref IA32PageTable exists for the given
       *   @ref IA32PageDirectory index, creating it if necessary.
       * @param pageDirectory
       *   Pointer to the @ref IA32PageDirectory.
       * @param pageDirectoryIndex
       *   Index of the @ref IA32PageDirectory entry to check.
       * @return
       *   Pointer to the ensured @ref IA32PageTable.
       */
      IA32PageTable* EnsureTable(
        IA32PageDirectory* pageDirectory,
        UInt32 pageDirectoryIndex
      );

      /**
       * @brief Gets the recursive @ref IA32PageDirectory mapping.
       * @return Pointer to the recursive @ref IA32PageDirectory.
       */
      inline IA32PageDirectory* GetRecursivePageDirectory() const {
        return _recursivePageDirectory;
      }

      /**
       * @brief Gets the kernel @ref IA32PageDirectory.
       * @return Pointer to the kernel @ref IA32PageDirectory.
       */
      inline IA32PageDirectory* GetKernelPageDirectory() const {
        return _kernelPageDirectory;
      }

    private:
      /**
       * @brief Pointer to the @ref IA32CPUDriver.
       */
      IA32CPUDriver* _cpu = nullptr;

      /**
       * @brief Pointer to the @ref IA32MemoryAllocator.
       */
      IA32MemoryAllocator* _memoryAllocator = nullptr;

      /**
       * @brief Pointer to the recursive @ref IA32PageDirectory mapping.
       */
      IA32PageDirectory* _recursivePageDirectory
        = reinterpret_cast<IA32PageDirectory*>(
            RECURSIVE_PAGE_DIRECTORY_VIRTUAL_ADDRESS
          );

      /**
       * @brief Pointer to the kernel @ref IA32PageDirectory.
       */
      IA32PageDirectory* _kernelPageDirectory = nullptr;

      /**
       * @brief Initializes the kernel @ref IA32PageDirectory.
       */
      void _initializeKernelPageDirectory();

      /**
       * @brief Initializes the kernel heap page tables.
       */
      void _initializeKernelHeapPageTables();

      /**
       * @brief Copies the kernel @ref IA32PageDirectory entries to a new
       *        @ref IA32PageDirectory.
       * @param newPageDirectory Pointer to the new @ref IA32PageDirectory.
       * @return `true` on success; `false` if a page table clone allocation
       *         failed (all partially cloned tables are rolled back).
       */
      bool _copyKernelPageDirectoriesTo(IA32PageDirectory* newPageDirectory);
  };
}
