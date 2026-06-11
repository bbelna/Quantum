/**
 * @file Kernel/Arch/IA32/Memory/IA32GDTEntry.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::IA32GDTEntry.
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
   * @brief An entry in the IA-32 Global Descriptor Table (GDT).
   *
   * Each 8-byte descriptor encodes a segment's base address, limit,
   * access rights, and granularity.
   *
   * The Quantum kernel uses a flat memory model with five descriptors: `null`,
   * kernel code (`0x08`), kernel data (`0x10`), user code
   * (`0x18 | RPL 3 = 0x1B`), user data (`0x20 | RPL 3 = 0x23`), and a TSS
   * descriptor (`0x28`).
   */
  struct [[gnu::packed]] IA32GDTEntry {
    /**
     * @brief Low 16 bits of the segment limit.
     */
    UInt16 LimitLow;

    /**
     * @brief Low 16 bits of the segment base address.
     */
    UInt16 BaseLow;

    /**
     * @brief Bits 16-23 of the segment base address.
     */
    UInt8 BaseMid;

    /**
     * @brief Access byte controlling segment type and privilege.
     *
     * Bit layout: P (present) | DPL (2-bit privilege) | S (descriptor
     * type: `1` = code/data, `0` = system) | Type (4-bit segment type).
     */
    UInt8 Access;

    /**
     * @brief Granularity byte and high nibble of the segment limit.
     *
     * Bit layout: G (granularity: `1` = 4 KB pages) | D/B (default
     * operand size: `1` = 32-bit) | L (64-bit, always `0` on IA-32) |
     * AVL (available) | LimitHigh (bits 16-19 of limit).
     */
    UInt8 Granularity;

    /**
     * @brief Bits 24-31 of the segment base address.
     */
    UInt8 BaseHigh;
  };
}
