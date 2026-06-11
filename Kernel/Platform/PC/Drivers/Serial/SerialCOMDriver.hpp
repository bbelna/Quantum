/**
 * @file Kernel/Platform/PC/Drivers/Serial/SerialCOMDriver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Serial::SerialCOMDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/ISA.hpp>

#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Platform::PC::Drivers::Serial {

  /**
   * @brief Kernel-level serial COM port driver.
   *
   * Initializes the specified 16550-compatible UART to 115200 baud, 8N1,
   * with no flow control. Provides polled (busy-wait) character and
   * string output used primarily by the kernel serial log sink.
   */
  class SerialCOMDriver : public IDriver {
    public:
      /**
       * @brief Standard PC serial COM port base addresses.
       */
      enum class Port : UInt16 {
        /** @brief COM1 base I/O port address (0x3F8). */
        COM1 = 0x3F8,

        /** @brief COM2 base I/O port address (0x2F8). */
        COM2 = 0x2F8,

        /** @brief COM3 base I/O port address (0x3E8). */
        COM3 = 0x3E8,

        /** @brief COM4 base I/O port address (0x2E8). */
        COM4 = 0x2E8
      };

      /**
       * @brief Creates a new `SerialCOMDriver` instance and initializes the
       *        specified port.
       * @param port The COM port to initialize.
       */
      explicit SerialCOMDriver(Port port);

      /**
       * @brief Destroys the `SerialCOMDriver` instance.
       */
      virtual ~SerialCOMDriver() = default;

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
       * @brief Writes a character to the serial port.
       * @param character The character to write.
       */
      void Write(char character);

      /**
       * @brief Writes a null-terminated string to the serial port.
       * @param string The string to write.
       */
      void Write(const char* string);

    private:
      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        4,
        "SerialCOM1",
        "Serial COM1 Port",
        ToDeviceCategoryID(DeviceCategoryType::SerialPort),
        DeviceState::Active,
        0,
        0,
        DeviceBus::ISA,
        0,
        {}
      };

      /**
       * @brief The base I/O port address for this COM port.
       */
      UInt16 _basePort;

      /**
       * @brief Checks if the transmit holding register is empty.
       * @return `true` when a character can be transmitted; `false` otherwise.
       */
      bool _canTransmit();
  };
}
