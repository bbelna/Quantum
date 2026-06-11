/**
 * @file Kernel/Arch/IA32/UserMode/Constants.hpp
 * @brief Declares @ref @QKrnlIA32::UserMode constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Kernel::Arch::IA32::UserMode {
  /**
   * @brief User code segment selector (ring 3).
   */
  constexpr UInt16 UserCodeSelector = 0x1B;

  /**
   * @brief User data segment selector (ring 3).
   */
  constexpr UInt16 UserDataSelector = 0x23;

  /**
   * @brief User stack segment selector (ring 3).
   * 
   * Same as user data segment on IA-32.
   */
  constexpr UInt16 UserStackSelector = UserDataSelector;

  /**
   * @brief Default EFLAGS for user mode (interrupts enabled, reserved bit 1
   *        set).
   *
   * Bit 1 is architecturally reserved and must always be 1. Bit 9 (IF)
   * enables maskable interrupts so the scheduler can preempt user threads.
   */
  constexpr UInt32 UserModeEFLAGS = 0x202;
}
