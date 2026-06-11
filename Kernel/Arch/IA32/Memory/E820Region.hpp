/**
 * @file Kernel/Arch/IA32/Memory/E820Region.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::E820Region.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Represents an E820 memory map region.
   *
   * Mirrors the BIOS `INT 15h`, `AX=E820h` entry layout. Each region
   * describes a contiguous range of physical memory with a type code:
   * `1` = usable RAM, `2` = reserved, `3` = ACPI reclaimable, `4` = ACPI NVS,
   * `5` = bad memory.
   */
  struct E820Region {
    /**
     * @brief Base physical address of the E820 region.
     */
    UInt32 BaseLow;

    /**
     * @brief High 32 bits of the base physical address of the E820 region.
     */
    UInt32 BaseHigh;

    /**
     * @brief Length of the E820 region in bytes.
     */
    UInt32 LengthLow;

    /**
     * @brief High 32 bits of the length of the E820 region.
     */
    UInt32 LengthHigh;

    /**
     * @brief Type of the E820 region.
     */
    UInt32 Type;
  };
}
