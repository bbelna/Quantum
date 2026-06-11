/**
 * @file Kernel/Drivers/Timers/ITimerDriver.hpp
 * @brief Declares @ref @QKrnl::Drivers::Timers::ITimerDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Drivers::Timers {
  /**
   * @brief Abstract device driver interface for a timer device.
   *
   * Concrete implementations (e.g., @ref Intel8253Driver) program a hardware
   * timer to fire periodic interrupts and expose a @ref Tick method that
   * the kernel's timer interrupt handler calls on each IRQ.
   */
  class ITimerDriver : public IDriver {
    public:
      /**
       * @brief Destroys this @ref ITimerDriver instance.
       */
      virtual ~ITimerDriver() = default;

      /**
       * @brief Handles a tick of the timer.
       * @param context Reference to the the current @ref IInterruptContext.
       * @return The next @ref IInterruptContext to switch to (possibly
       *         a different thread).
       */
      virtual IInterruptContext* Tick(IInterruptContext& context) = 0;
  };
}
