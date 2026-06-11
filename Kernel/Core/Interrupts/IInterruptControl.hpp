/**
 * @file Kernel/Interrupts/IInterruptControl.hpp
 * @brief Declares @ref @QKrnl::Interrupts::IInterruptControl.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Interrupts {
  /**
   * @brief CRTP interface for architecture-specific interrupt control.
   * @tparam FlagsType The type used to save interrupt state (e.g.,
   *                   `UInt32` for IA-32 EFLAGS).
   * @tparam Impl The concrete architecture-specific implementation (e.g.,
   *              @ref IA32InterruptControl).
   */
  template <typename FlagsType, typename Impl>
  class IInterruptControl {
    public:
      /**
       * @brief The architecture-specific type used to save interrupt
       *        state.
       */
      using Flags = FlagsType;

      /**
       * @brief Saves the current interrupt state and disables interrupts.
       * @return The saved interrupt flags, to be passed to @ref Restore.
       */
      static inline Flags SaveAndDisable() {
        return Impl::DoSaveAndDisable();
      }

      /**
       * @brief Restores interrupt state from previously saved flags.
       * @param flags The flags returned by @ref SaveAndDisable.
       */
      static inline void Restore(Flags flags) {
        Impl::DoRestore(flags);
      }

      /**
       * @brief Issues a processor hint for spin-wait loops.
       *
       * On architectures that support it (e.g., IA-32 `PAUSE`), this
       * reduces power consumption and improves performance of tight
       * spin loops.
       */
      static inline void SpinHint() {
        Impl::DoSpinHint();
      }
  };
}
