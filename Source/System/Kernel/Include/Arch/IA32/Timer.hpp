/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Timer.hpp
 * IA32 PIT timer driver.
 */

#pragma once

#include <Interrupts.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
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

    private:
      /**
       * PIT channel 0 data port.
       */
      static constexpr UInt16 _pitChannel0 = 0x40;

      /**
       * PIT command port.
       */
      static constexpr UInt16 _pitCommand = 0x43;

      /**
       * PIT input clock frequency in Hz.
       */
      static constexpr UInt32 _pitInputHz = 1193180;

      /**
       * PIT operating mode configuration.
       */
      static constexpr UInt16 _pitMode = 0x36;

      /**
       * Desired PIT frequency in Hz.
       */
      static constexpr UInt32 _pitFreqHz = 100;

      /**
       * Tick count since timer initialization.
       */
      inline static volatile UInt64 _tickCount = 0;

      /**
       * Whether periodic tick logging is enabled.
       */
      inline static volatile bool _tickLoggingEnabled = false;

      /**
       * PIT timer interrupt handler.
       */
      static Interrupts::Context* TimerHandler(
        Interrupts::Context& context
      );
  };
}
