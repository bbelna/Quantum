/**
 * @file Kernel/Arch/IA32/Memory/IA32PageDirectory.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::PageDirectory.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>
#include <Memory/IAddressSpace.hpp>

#include "IA32PageConstants.hpp"
#include "IA32PageDirectoryEntry.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 page directory.
   *
   * Non-PAE, 1024 entries, 4 KB aligned. Each entry either points to a
   * page table (4 KB pages) or maps a 4 MB large page directly. The
   * last entry (index 1023) is reserved for the recursive mapping that
   * allows software access to page tables without identity-mapping them.
   *
   * Inherits from @ref IAddressSpace so core kernel code can reference
   * it without arch-specific headers.
   */
  struct alignas(PAGE_SIZE) IA32PageDirectory : public IAddressSpace {
    /**
     * @brief Computes the page directory index for a process address.
     * @param virtualAddress The process address.
     * @return The page directory index (`0`-`1023`).
     */
    static UInt32 Index(UIntPtr virtualAddress) {
      return (virtualAddress >> 22) & 0x3FFu;
    }

    /**
     * @brief The page directory entries.
     */
    IA32PageDirectoryEntry Entries[PAGE_DIRECTORY_ENTRY_COUNT];
  };

  static_assert(sizeof(IA32PageDirectory) == PAGE_SIZE);
}
