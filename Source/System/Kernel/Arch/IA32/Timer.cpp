/**
 * @file System/Kernel/Arch/IA32/Timer.cpp
 * @brief IA32 PIT timer driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/PIC.hpp"
#include "Arch/IA32/Task.hpp"
#include "Arch/IA32/Timer.hpp"
#include "Interrupts.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Kernel::Logger::Level;

  Interrupts::Context* Timer::TimerHandler(Interrupts::Context& context) {
    ++_tickCount;

    // heartbeat every second (at 100 Hz)
    if (_tickLoggingEnabled && (_tickCount % _pitFreqHz) == 0) {
      Logger::Write(LogLevel::Trace, "Tick");
    }

    return Task::Tick(context);
  }

  void Timer::Initialize() {
    // program PIT for desired frequency
    UInt16 divisor = static_cast<UInt16>(_pitInputHz / _pitFreqHz);
    IO::Out8(_pitCommand, _pitMode);
    IO::Out8(_pitChannel0, divisor & 0xFF);
    IO::Out8(_pitChannel0, (divisor >> 8) & 0xFF);

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
