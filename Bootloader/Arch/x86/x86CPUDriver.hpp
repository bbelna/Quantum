/**
 * @file Bootloader/Arch/x86/x86CPUDriver.hpp
 * @brief Declares @ref @QBtldr::Arch::x86::x86CPUDriver.
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
   * @brief x86 implementation of @ref @QBtldr::ICPUDriver.
   */
  class x86CPUDriver : public ICPUDriver {
    public:
      /**
       * @brief Disables interrupts and spins on `hlt` forever.
       *
       * Emits `cli` followed by an infinite `hlt` / `jmp` loop using inline
       * assembly. Called on unrecoverable boot failures.
       */
      [[noreturn]] void HaltForever() override;
  };
}
