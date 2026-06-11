/**
 * @file Drivers/Input/PS2Mouse/PS2MouseDriver.hpp
 * @brief Declares @ref @QDrvs::Input::PS2Mouse::PS2MouseDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <PS2MouseDriverTypes.hpp>

namespace Quantum::Drivers::Input::PS2Mouse {
  /**
   * @brief PS/2 mouse hardware driver that handles low-level port I/O and
   *        assembles mouse packets into input events. Contains no IPC or
   *        lifecycle logic.
   */
  class PS2MouseDriver {
    public:
      /**
       * @brief PS/2 data port.
       */
      static constexpr UInt16 DataPort = 0x60;

      /**
       * @brief PS/2 controller command/status port.
       */
      static constexpr UInt16 CommandPort = 0x64;

      /**
       * @brief PS/2 mouse IRQ line.
       */
      static constexpr UInt8 IRQLine = 12;

      /**
       * @brief Device ID for the PS/2 mouse.
       */
      static constexpr DeviceID DeviceIdentifier = 102;

      /**
       * @brief Constructs the PS/2 mouse driver.
       */
      explicit PS2MouseDriver() = default;

      /**
       * @brief Requests PortIO and Interrupt permissions, initializes the
       *        PS/2 mouse hardware, and claims IRQ 12.
       * @return `true` if all initialization succeeded; `false` on failure.
       */
      bool Initialize();

      /**
       * @brief Returns the resource handle for the claimed IRQ.
       * @return The IRQ resource handle.
       */
      ResourceID GetIRQHandle() const;

      /**
       * @brief Feeds a single data byte into the packet buffer. When a
       *        complete packet is assembled, processes it into input events.
       * @param data The byte read from the data port.
       * @param outEvents Pointer to an array of InputEvent to fill.
       * @param maxEvents Maximum number of events that can be written.
       * @return The number of events produced (0 if the packet is incomplete
       *         or was discarded).
       */
      Size ProcessByte(
        UInt8 data,
        InputEvent* outEvents,
        Size maxEvents
      );

    private:
      /**
       * @brief Handle for the claimed IRQ resource.
       */
      ResourceID _irqHandle = static_cast<ResourceID>(-1);

      /**
       * @brief Accumulated packet bytes.
       */
      UInt8 _packetBuffer[4] = {};

      /**
       * @brief Current index into the packet buffer.
       */
      UInt8 _packetIndex = 0;

      /**
       * @brief Number of bytes per packet (3 for standard, 4 for
       *        IntelliMouse with scroll wheel).
       */
      UInt8 _packetSize = 3;

      /**
       * @brief Whether the mouse supports the IntelliMouse protocol
       *        (scroll wheel).
       */
      bool _hasScrollWheel = false;

      /**
       * @brief Previous button state for detecting changes.
       */
      UInt8 _lastButtons = 0;

      /**
       * @brief Initializes the PS/2 mouse hardware.
       */
      void _initializeMouse();

      /**
       * @brief Waits for the controller input buffer to be empty.
       * @return `true` if the buffer became empty; `false` on timeout.
       */
      bool _waitForInput();

      /**
       * @brief Waits for the controller output buffer to have data.
       * @return `true` if data became available; `false` on timeout.
       */
      bool _waitForOutput();

      /**
       * @brief Sends a command byte to the mouse via the `0xD4` prefix.
       * @param command The command byte to send.
       */
      void _sendMouseCommand(UInt8 command);

      /**
       * @brief Sends a Set Sample Rate command (`0xF3`) with the given rate.
       * @param rate The sample rate value.
       */
      void _setSampleRate(UInt8 rate);

      /**
       * @brief Reads the mouse device ID via Get Device ID (`0xF2`).
       * @return The device ID (`0` = standard, `3` = IntelliMouse).
       */
      UInt8 _getDeviceID();

      /**
       * @brief Processes a complete mouse packet into input events.
       * @param outEvents Pointer to an array of InputEvent to fill.
       * @param maxEvents Maximum number of events that can be written.
       * @return The number of events produced.
       */
      Size _processPacket(InputEvent* outEvents, Size maxEvents);
  };
}
