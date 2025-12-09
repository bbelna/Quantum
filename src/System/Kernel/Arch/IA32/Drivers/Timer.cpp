//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/Timer.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// PIT timer driver.
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <KernelTypes.hpp>
#include <Drivers/Console.hpp>
#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>
#include <Arch/IA32/Drivers/Timer.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace {
    /**
     * PIT channel 0 data port.
     */
    constexpr uint16 pitChannel0 = 0x40;

    /**
     * PIT command port.
     */
    constexpr uint16 pitCommand = 0x43;

    /**
     * PIT input clock frequency in Hz.
     */
    constexpr uint32 pitInputHz = 1193180;

    /**
     * PIT operating mode configuration.
     */
    constexpr uint16 pitMode = 0x36;

    /**
     * Desired PIT frequency in Hz.
     */
    constexpr uint32 pitFreqHz  = 100;

    /**
     * Tick count since timer initialization.
     */
    volatile uint64 tickCount = 0;
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
    uint16 divisor = static_cast<uint16>(pitInputHz / pitFreqHz);
    IO::OutByte(pitCommand, pitMode);
    IO::OutByte(pitChannel0, divisor & 0xFF);
    IO::OutByte(pitChannel0, (divisor >> 8) & 0xFF);

    // register IRQ0 handler and unmask
    Interrupts::RegisterHandler(32, TimerHandler); // IRQ0 vector
    PIC::Unmask(0);
  }

  uint64 Timer::Ticks() {
    return tickCount;
  }

  void Timer::SetTickLoggingEnabled(bool enabled) {
    tickLoggingEnabled = enabled;
  }
}
