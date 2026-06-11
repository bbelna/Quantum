/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptControl.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32InterruptControl.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include <Interrupts/IInterruptControl.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief IA-32 implementation of @ref IInterruptControl.
   *
   * Saves and restores the interrupt-enable flag (bit 9 of EFLAGS) and
   * issues the `PAUSE` hint for spin-wait loops.
   */
  class IA32InterruptControl : public IInterruptControl<
    UInt32,
    IA32InterruptControl
  > {
    friend class IInterruptControl<
      UInt32,
      IA32InterruptControl
    >;

    private:
      /**
       * @brief Saves EFLAGS and disables interrupts via `CLI`.
       * @return The saved EFLAGS value.
       */
      static inline UInt32 DoSaveAndDisable() {
        UInt32 flags;

        asm volatile(
          "pushfl\n"
          "pop %0\n"
          "cli"
          : "=r"(flags) : : "memory"
        );

        return flags;
      }

      /**
       * @brief Restores the interrupt-enable flag from saved EFLAGS.
       * @param flags The EFLAGS value saved by @ref DoSaveAndDisable.
       *
       * Re-enables interrupts only if they were enabled at save time
       * (bit 9 — IF — was set).
       */
      static inline void DoRestore(UInt32 flags) {
        if (flags & 0x200) asm volatile("sti" ::: "memory");
      }

      /**
       * @brief Issues the IA-32 `PAUSE` instruction for spin-wait loops.
       */
      static inline void DoSpinHint() {
        asm volatile("pause");
      }
  };

  /**
   * @brief Architecture-agnostic alias for @ref IA32InterruptControl.
   */
  using InterruptControl = IA32InterruptControl;
}
