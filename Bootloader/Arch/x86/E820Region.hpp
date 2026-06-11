/**
 * @file Bootloader/Arch/x86/E820Region.hpp
 * @brief Declares @ref @QBtldr::Arch::x86::E820Region.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/x86/x86Types.hpp>

namespace Quantum::Bootloader::Arch::x86 {
  /**
   * @brief A single E820 memory map entry.
   *
   * Describes a region of physical address space with its type (usable,
   * reserved, ACPI reclaimable, etc.). Each entry is 20 bytes, matching
   * the `INT 15h`/E820 output format.
   */
  struct E820Region {
    /**
     * @brief Low 32 bits of the region base address.
     */
    UInt32 BaseLow;

    /**
     * @brief High 32 bits of the region base address.
     */
    UInt32 BaseHigh;

    /**
     * @brief Low 32 bits of the region length.
     */
    UInt32 LengthLow;

    /**
     * @brief High 32 bits of the region length.
     */
    UInt32 LengthHigh;

    /**
     * @brief Region type (`1` = usable, `2` = reserved, `3` = ACPI
     *        reclaimable, `4` = ACPI NVS, `5` = bad memory).
     */
    UInt32 Type;
  };
}
