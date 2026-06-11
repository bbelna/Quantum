/**
 * @file Kernel/Interrupts/IInterruptManager.hpp
 * @brief Declares @ref @QKrnl::Interrupts::IInterruptManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IInterruptContext.hpp"
#include "InterruptTypes.hpp"

namespace Quantum::Kernel::Interrupts {
  /**
   * @brief Abstract interface for the kernel interrupt manager.
   *
   * Provides architecture-agnostic operations for registering interrupt
   * handlers, dispatching exceptions, and supplying the kernel context
   * to interrupt service routines.
   */
  template <typename InterruptVectorType>
  class IInterruptManager {
    public:
      /**
       * @brief Registers an @ref InterruptHandler for the given
       *        @ref InterruptVector.
       * @param vector The @ref InterruptVector to register the
       *               @ref InterruptHandler for.
       * @param handler The @ref InterruptHandler to invoke when an
       *                interrupt with the given @p vector occurs.
       */
      virtual void SetHandler(
        InterruptVectorType vector,
        InterruptHandler handler
      ) = 0;

      /**
       * @brief Handles an interrupt.
       * @param context Pointer to the @ref IInterruptContext at the time of
       *                the interrupt.
       * @return The next @ref IInterruptContext to restore. May differ from the
       *         input if the interrupt triggers a @ref Thread kill or context
       *         switch.
       */
      virtual IInterruptContext* HandleException(
        IInterruptContext* context
      ) = 0;
  };
}
