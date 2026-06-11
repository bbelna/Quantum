/**
 * @file Kernel/Arch/IA32/Interrupts/IA32IDTDescriptor.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32IDTDescriptor.
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
    * @brief IA-32 IDT descriptor for the `lidt` instruction.
    */
  struct [[gnu::packed]] IA32IDTDescriptor {
    /**
      * @brief Size of the IDT in bytes minus one.
      */
    UInt16 Limit;

    /**
      * @brief Linear base address of the IDT.
      */
    UInt32 Base;
  };
}
