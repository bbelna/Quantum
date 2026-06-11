/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptTypes.hpp
 * @brief Declares @QKrnlIA32::Interrupts types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  struct IA32InterruptDescriptor;
  struct IA32InterruptEntry;
  struct IA32InterruptContext;
  struct IA32InterruptResource;

  class IA32IDT;
  class IA32InterruptControl;
  class IA32InterruptDispatcher;
  class IA32SystemCallHandler;

  /**
   * @brief Type alias for an interrupt vector number (0-255).
   *
   * On IA-32, the IDT supports 256 entries. Vectors 0-31 are CPU
   * exceptions, 32-47 are hardware IRQs (after PIC remapping), 49 is
   * the software yield vector, and 128 (0x80) is the system call gate.
   */
  using IA32InterruptVector = UInt8;
}

using namespace Quantum::Kernel::Arch::IA32::Interrupts;
