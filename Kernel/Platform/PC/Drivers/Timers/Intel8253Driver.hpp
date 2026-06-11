/**
 * @file Kernel/Platform/PC/Drivers/Timers/Intel8253Driver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Timers::Intel8253Driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/ISA.hpp>

#include <Drivers/DriverTypes.hpp>
#include <Drivers/Timers/ITimerDriver.hpp>
#include <KernelTypes.hpp>

/**
 * @brief PIT base oscillator frequency in Hz (1.193182 MHz).
 */
#define PIT_BASE_FREQUENCY 1193182

/**
 * @brief PIT Channel 0 data port (system timer).
 */
#define PIT_CHANNEL0_PORT 0x40

/**
 * @brief PIT Channel 1 data port (legacy DRAM refresh).
 */
#define PIT_CHANNEL1_PORT 0x41

/**
 * @brief PIT Channel 2 data port (PC speaker).
 */
#define PIT_CHANNEL2_PORT 0x42

/**
 * @brief PIT command/mode register port.
 */
#define PIT_COMMAND_PORT 0x43

/**
 * @brief PIT command: Channel 0, lobyte/hibyte, rate generator mode.
 */
#define PIT_CMD_CHANNEL0 0x00

/**
 * @brief PIT command: Access mode lobyte/hibyte.
 */
#define PIT_CMD_LOHIBYTE 0x30

/**
 * @brief PIT command: Mode 2 (rate generator).
 */
#define PIT_CMD_MODE2 0x04

/**
 * @brief PIT command: Mode 3 (square wave generator).
 */
#define PIT_CMD_MODE3 0x06

/**
 * @brief PIT command: Binary counting (not BCD).
 */
#define PIT_CMD_BINARY 0x00

namespace Quantum::Kernel::Platform::PC::Drivers::Timers {
  /**
   * @brief Intel 8253/8254 Programmable Interval Timer (PIT) driver.
   *
   * Programs PIT Channel 0 in rate-generator mode (Mode 2) to fire
   * periodic interrupts at the requested frequency. The base oscillator
   * runs at 1.193182 MHz; the reload value is computed as
   * `PIT_BASE_FREQUENCY / frequencyHz`. The driver also maintains a
   * monotonic tick counter incremented on each IRQ.
   */
  class Intel8253Driver : public ITimerDriver {
    public:
      /**
       * @brief Default timer frequency in Hz (1000 Hz = 1ms intervals).
       */
      static constexpr UInt32 DefaultFrequency = 1000;

      /**
       * @brief Minimum supported frequency in Hz.
       */
      static constexpr UInt32 MinFrequency = 19;

      /**
       * @brief Maximum supported frequency in Hz.
       */
      static constexpr UInt32 MaxFrequency = PIT_BASE_FREQUENCY;

      /**
       * @brief Constructs and initializes the PIT with the specified frequency.
       * @param frequencyHz Desired interrupt frequency in Hz.
       * @return The initialized PIT instance.
       */
      explicit Intel8253Driver(
        UInt32 frequencyHz = DefaultFrequency
      );

      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      Device& GetDevice() override { return _device; }

      /**
       * @brief Gets the unique identifier of the device.
       * @return The device ID.
       */
      DeviceID GetDeviceID() override { return _device.ID; }

      /**
       * @brief Invokes a driver-specific operation.
       * @param operation The operation code.
       * @param payload Optional payload pointer.
       * @return A driver-specific result value.
       */
      UInt32 Invoke(UInt32 operation, void* payload) override { return 0; }

      /**
       * @brief Gets the current configured frequency.
       * @return The frequency in Hz.
       */
      UInt32 GetFrequency() const;

      /**
       * @brief Gets the total number of ticks since initialization.
       * @return The tick count.
       */
      UInt64 GetTickCount() const;

      /**
       * @brief Increments the tick counter. Called by the timer interrupt.
       * @param context The current interrupt context.
       * @return The next context to switch to. Currently always returns
       *         the same context.
       */
      IInterruptContext* Tick(
        IInterruptContext& context
      ) override;

      /**
       * @brief Converts ticks to milliseconds.
       * @param ticks Number of ticks.
       * @return Equivalent milliseconds.
       */
      UInt64 TicksToMilliseconds(UInt64 ticks) const;

      /**
       * @brief Converts milliseconds to ticks.
       * @param milliseconds Number of milliseconds.
       * @return Equivalent ticks.
       */
      UInt64 MillisecondsToTicks(UInt64 milliseconds) const;

    private:
      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        2,
        "Intel8253",
        "Intel 8253 Programmable Interval Timer",
        ToDeviceCategoryID(DeviceCategoryType::Controller),
        DeviceState::Active,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief Current configured frequency in Hz.
       */
      UInt32 _frequency = 0;

      /**
       * @brief Total tick count since initialization.
       */
      UInt64 _tickCount = 0;

  };
}
