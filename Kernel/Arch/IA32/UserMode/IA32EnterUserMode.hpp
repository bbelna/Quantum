/**
 * @file Kernel/Arch/IA32/UserMode/IA32EnterUserMode.hpp
 * @brief Declares @ref @QKrnlIA32::UserMode::IA32EnterUserMode.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::UserMode {
  /**
   * @brief Enters user mode via `iret`.
   * @param userCS User code segment selector.
   * @param userSS User stack segment selector.
   * @param userEIP User instruction pointer.
   * @param userESP User stack pointer.
   * @param eflags `EFLAGS` value for user mode.
   * 
   * This is implemented in assembly. It sets up the stack for iret
   * to transition to user mode (ring 3).
   */
  extern "C"
  [[noreturn]]
  void EnterUserMode(
    UInt32 userCS,
    UInt32 userSS,
    UInt32 userEIP,
    UInt32 userESP,
    UInt32 eflags
  );
}
