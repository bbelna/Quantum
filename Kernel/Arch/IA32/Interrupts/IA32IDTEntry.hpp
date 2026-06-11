/**
 * @file Kernel/Arch/IA32/Interrupts/IA32IDTEntry.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32IDTEntry.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief IA-32 IDT entry representing a gate descriptor.
   */
  struct [[gnu::packed]] IA32IDTEntry {
    /**
      * @brief Bits 0-15 of the interrupt handler address.
      */
    UInt16 OffsetLow;

    /**
      * @brief Code segment selector.
      */
    UInt16 Selector;

    /**
      * @brief Always zero.
      */
    UInt8 Zero;

    /**
      * @brief Type and attributes (present, DPL, gate type).
      */
    UInt8 TypeAttribute;

    /**
      * @brief Bits 16-31 of the interrupt handler address.
      */
    UInt16 OffsetHigh;
  };
}
