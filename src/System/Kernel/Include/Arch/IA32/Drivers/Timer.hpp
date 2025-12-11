//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/Timer.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// PIT timer driver for IA32.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  /**
   * IA32 PIT timer driver class.
   */
  class Timer {
    public:
      /**
       * Initializes the PIT timer to a fixed frequency and registers IRQ0.
       */
      static void Initialize();

      /**
       * Returns the current tick count since timer init.
       */
      static UInt64 Ticks();

      /**
       * Enables or disables periodic tick logging to the console.
       */
      static void SetTickLoggingEnabled(bool enabled);
  };
}
