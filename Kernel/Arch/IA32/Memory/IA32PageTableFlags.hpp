/**
 * @file Kernel/Arch/IA32/Memory/IA32PageTableFlags.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::PageTableFlag.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 page table entry bit flags.
   *
   * Each flag corresponds to a hardware-defined bit in the 32-bit page
   * table entry. Bitmask operators are enabled via
   * `QUANTUM_ENABLE_ENUM_BITMASK_OPS` so flags can be combined with `|`
   * and tested with `HasAnyFlag`.
   */
  enum class IA32PageTableFlags : UInt32 {
    /**
     * @brief No page bits set.
     */
    None = 0,

    /**
     * @brief Page is present in memory.
     */
    Present = 1u << 0,

    /**
     * @brief Page is writable.
     */
    Write = 1u << 1,

    /**
     * @brief Page is accessible from user mode.
     */
    User = 1u << 2,

    /**
     * @brief Write-through caching enabled.
     */
    WriteThrough = 1u << 3,

    /**
     * @brief Cache disabled for this page.
     */
    CacheDisable = 1u << 4,

    /**
     * @brief Page has been accessed.
     */
    Accessed = 1u << 5,

    /**
     * @brief Page has been written to.
     */
    Dirty = 1u << 6,

    /**
     * @brief Page attribute table index.
     */
    PAT = 1u << 7,

    /**
     * @brief Global page (not flushed from TLB on CR3 reload).
     */
    Global = 1u << 8,

    /**
     * @brief Available for system programmer use.
     */
    Avl0 = 1u << 9,

    /**
     * @brief Available for system programmer use.
     */
    Avl1 = 1u << 10,

    /**
     * @brief Available for system programmer use.
     */
    Avl2 = 1u << 11,
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Kernel::Arch::IA32::Memory,
  IA32PageTableFlags
)
