/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Drivers/Timer.hpp
 * IA32 PIT timer driver.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  /**
   * IA32 PIT timer driver.
   */
  class Timer {
    public:
      /**
       * Initializes the PIT timer to a fixed frequency and registers IRQ0.
       */
      static void Initialize();

      /**
       * Returns the current tick count since timer init.
       * @return
       *   The current tick count.
       */
      static UInt64 Ticks();

      /**
       * Enables or disables periodic tick logging to the console.
       * @param enabled
       *   True to enable tick logging, false to disable.
       */
      static void SetTickLoggingEnabled(bool enabled);
  };
}
