/**
 * @file Kernel/Arch/IA32/Memory/IA32PageTable.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::PageTable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "IA32PageConstants.hpp"
#include "IA32PageTableEntry.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 page table.
   *
   * Non-PAE, 1024 entries, 4 KB aligned.
   */
  struct alignas(PAGE_SIZE) IA32PageTable {
    /**
     * @brief Computes the page table index for a process address.
     * @param virtualAddress The process address.
     * @return The page table index (`0`-`1023`).
     */
    static UInt32 Index(UIntPtr virtualAddress) {
      return (virtualAddress >> 12) & 0x3FFu;
    }

    /**
     * @brief The page table entries.
     */
    IA32PageTableEntry Entries[PAGE_TABLE_ENTRY_COUNT];
  };

  static_assert(sizeof(IA32PageTable) == PAGE_SIZE);
}
