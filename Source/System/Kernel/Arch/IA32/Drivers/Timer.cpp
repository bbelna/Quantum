/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Drivers/Timer.cpp
 * IA32 PIT timer driver.
 */

#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/PIC.hpp>
#include <Arch/IA32/Drivers/Timer.hpp>
#include <Interrupts.hpp>
#include <Logger.hpp>
#include <Types/Logging/Level.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Drivers {
  using LogLevel = Types::Logging::Level;

  namespace {
    /**
     * PIT channel 0 data port.
     */
    constexpr UInt16 _pitChannel0 = 0x40;

    /**
     * PIT command port.
     */
    constexpr UInt16 _pitCommand = 0x43;

    /**
     * PIT input clock frequency in Hz.
     */
    constexpr UInt32 _pitInputHz = 1193180;

    /**
     * PIT operating mode configuration.
     */
    constexpr UInt16 _pitMode = 0x36;

    /**
     * Desired PIT frequency in Hz.
     */
    constexpr UInt32 _pitFreqHz  = 100;

    /**
     * Tick count since timer initialization.
     */
    volatile UInt64 _tickCount = 0;

    /**
     * Whether periodic tick logging is enabled.
     */
    volatile bool _tickLoggingEnabled = false;

    /**
     * PIT timer interrupt handler.
     */
    void TimerHandler(InterruptContext&) {
      ++_tickCount;

      // heartbeat every second (at 100 Hz)
      if (_tickLoggingEnabled && (_tickCount % _pitFreqHz) == 0) {
        Logger::Write(LogLevel::Trace, "Tick");
      }
    }
  }

  void Timer::Initialize() {
    // program PIT for desired frequency
    UInt16 divisor = static_cast<UInt16>(_pitInputHz / _pitFreqHz);
    IO::OutByte(_pitCommand, _pitMode);
    IO::OutByte(_pitChannel0, divisor & 0xFF);
    IO::OutByte(_pitChannel0, (divisor >> 8) & 0xFF);

    // register IRQ0 handler and unmask
    Interrupts::RegisterHandler(32, TimerHandler); // IRQ0 vector
    PIC::Unmask(0);
  }

  UInt64 Timer::Ticks() {
    return _tickCount;
  }

  void Timer::SetTickLoggingEnabled(bool enabled) {
    _tickLoggingEnabled = enabled;
  }
}
