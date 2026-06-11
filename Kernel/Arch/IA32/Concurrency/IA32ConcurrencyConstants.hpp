/**
 * @file Kernel/Arch/IA32/Concurrency/IA32ConcurrencyConstants.hpp
 * @brief Declares @ref @QKrnlIA32::Concurrency constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief Kernel code segment selector.
   */
  constexpr UInt16 KernelCodeSegment = 0x08;

  /**
   * @brief Kernel data segment selector.
   */
  constexpr UInt16 KernelDataSegment = 0x10;

  /**
   * @brief User code segment selector.
   */
  constexpr UInt16 UserCodeSegment = 0x1B;

  /**
   * @brief User data segment selector.
   */
  constexpr UInt16 UserDataSegment = 0x23;

  /**
   * @brief Default EFLAGS value for new kernel threads (interrupts enabled).
   *
   * Bit 9 (IF) is set to enable maskable interrupts. Bit 1 (reserved,
   * always 1) is not set here because kernel threads rely on it being
   * set elsewhere or the CPU enforcing it.
   */
  constexpr UInt32 DefaultEFLAGS = 0x200;
}
