/**
 * @file Kernel/Platform/PC/Drivers/Timers/Intel8253Driver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Timers::Intel8253Driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "Intel8253Driver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Timers {
  using namespace Arch;

  Intel8253Driver::Intel8253Driver(UInt32 frequencyHz) {
    SetISAAddress(_device, { PIT_CHANNEL0_PORT, 0 });

    // clamp frequency to valid range
    if (frequencyHz < MinFrequency) frequencyHz = MinFrequency;
    else if (frequencyHz > MaxFrequency) frequencyHz = MaxFrequency;

    _frequency = frequencyHz;
    _tickCount = 0;

    // calculate the divisor
    UInt32 divisor = PIT_BASE_FREQUENCY / frequencyHz;

    // clamp divisor to 16-bit range
    if (divisor > 65535) divisor = 65535;
    else if (divisor < 1) divisor = 1;

    // send command: channel 0, lobyte/hibyte, rate generator, binary
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(
      PIT_CMD_CHANNEL0 | PIT_CMD_LOHIBYTE | PIT_CMD_MODE2 | PIT_CMD_BINARY
    )), "Nd"(static_cast<UInt16>(PIT_COMMAND_PORT)));

    // send divisor (low byte first, then high byte)
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(divisor & 0xFF)), "Nd"(static_cast<UInt16>(PIT_CHANNEL0_PORT)));
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>((divisor >> 8) & 0xFF)), "Nd"(static_cast<UInt16>(PIT_CHANNEL0_PORT)));

    KLOG_TRACE(
      "Intel 8253 kernel driver initialized at %u Hz (divisor %u, "
      "actual %u Hz)",
      frequencyHz,
      divisor,
      PIT_BASE_FREQUENCY / divisor
    );
  }

  UInt32 Intel8253Driver::GetFrequency() const { return _frequency; }

  UInt64 Intel8253Driver::GetTickCount() const { return _tickCount; }

  IInterruptContext* Intel8253Driver::Tick(
    IInterruptContext& context
  ) {
    _tickCount++;

    return &context;
  }

  UInt64 Intel8253Driver::TicksToMilliseconds(UInt64 ticks) const {
    if (_frequency == 0) return 0;

    return (ticks * 1000) / _frequency;
  }

  UInt64 Intel8253Driver::MillisecondsToTicks(UInt64 milliseconds) const {
    return (milliseconds * _frequency) / 1000;
  }
}
