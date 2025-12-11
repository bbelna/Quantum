//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/Timer.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// PIT timer driver.
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <Types.hpp>
#include <Drivers/Console.hpp>
#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>
#include <Arch/IA32/Drivers/Timer.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace {
    /**
     * PIT channel 0 data port.
     */
    constexpr UInt16 pitChannel0 = 0x40;

    /**
     * PIT command port.
     */
    constexpr UInt16 pitCommand = 0x43;

    /**
     * PIT input clock frequency in Hz.
     */
    constexpr UInt32 pitInputHz = 1193180;

    /**
     * PIT operating mode configuration.
     */
    constexpr UInt16 pitMode = 0x36;

    /**
     * Desired PIT frequency in Hz.
     */
    constexpr UInt32 pitFreqHz  = 100;

    /**
     * Tick count since timer initialization.
     */
    volatile UInt64 tickCount = 0;
    volatile bool tickLoggingEnabled = false;

    /**
     * PIT timer interrupt handler.
     */
    void TimerHandler(InterruptContext&) {
      ++tickCount;

      // Heartbeat every second (at 100 Hz).
      if (tickLoggingEnabled && (tickCount % pitFreqHz) == 0) {
        Quantum::Kernel::Drivers::Console::WriteLine("Tick");
      }
    }
  }

  void Timer::Initialize() {
    // program PIT for desired frequency
    UInt16 divisor = static_cast<UInt16>(pitInputHz / pitFreqHz);
    IO::OutByte(pitCommand, pitMode);
    IO::OutByte(pitChannel0, divisor & 0xFF);
    IO::OutByte(pitChannel0, (divisor >> 8) & 0xFF);

    // register IRQ0 handler and unmask
    Interrupts::RegisterHandler(32, TimerHandler); // IRQ0 vector
    PIC::Unmask(0);
  }

  UInt64 Timer::Ticks() {
    return tickCount;
  }

  void Timer::SetTickLoggingEnabled(bool enabled) {
    tickLoggingEnabled = enabled;
  }
}
