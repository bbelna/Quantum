/**
 * @file Services/Drivers/Input/PS2/Controller/Include/Controller.hpp
 * @brief PS/2 controller helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::Services::Drivers::Input::PS2 {
  /**
   * PS/2 controller I/O helpers.
   */
  class Controller {
    public:
      /**
       * Initializes the controller interface.
       * @return
       *   True on success; false otherwise.
       */
      static bool Initialize();

      /**
       * Reads a byte from the PS/2 data port.
       * @return
       *   Byte read from the controller.
       */
      static UInt8 ReadData();

      /**
       * Writes a command to the PS/2 controller.
       * @param command
       *   Command byte.
       * @return
       *   True on success; false on timeout.
       */
      static bool WriteCommand(UInt8 command);

      /**
       * Writes a byte to the PS/2 data port.
       * @param value
       *   Byte to write.
       * @return
       *   True on success; false on timeout.
       */
      static bool WriteData(UInt8 value);

    private:
      /**
       * PS/2 data port.
       */
      static constexpr UInt16 _dataPort = 0x60;

      /**
       * PS/2 status/command port.
       */
      static constexpr UInt16 _statusPort = 0x64;

      /**
       * PS/2 command port.
       */
      static constexpr UInt16 _commandPort = 0x64;

      /**
       * Status bit: output buffer full.
       */
      static constexpr UInt8 _statusOutputFull = 1u << 0;

      /**
       * Status bit: input buffer full.
       */
      static constexpr UInt8 _statusInputFull = 1u << 1;

      /**
       * Waits for data availability.
       * @return
       *   True if data is available; false on timeout.
       */
      static bool WaitForRead();

      /**
       * Waits for the controller input buffer to clear.
       * @return
       *   True if ready for write; false on timeout.
       */
      static bool WaitForWrite();
  };
}
