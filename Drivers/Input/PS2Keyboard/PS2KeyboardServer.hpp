/**
 * @file Drivers/Input/PS2Keyboard/PS2KeyboardServer.hpp
 * @brief Declares @ref @QDrvs::Input::PS2Keyboard::PS2KeyboardServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "PS2KeyboardDriver.hpp"

namespace Quantum::Drivers::Input::PS2Keyboard {
  /**
   * @brief Interrupt-driven event loop that initializes the PS/2 keyboard
   *        hardware, registers the device, and forwards scancode events to
   *        the input server.
   *
   * At startup the server:
   *   1. Calls @ref PS2KeyboardDriver::Initialize to acquire permissions and
   *      claim IRQ 1.
   *   2. Registers the PS/2 keyboard device with the device server.
   *   3. Opens a persistent send handle to the input server.
   *   4. Indicates to the startup server that initialization is complete and
   *      the driver is ready.
   *   5. Enters the IRQ loop, reading scancodes and reporting input events.
   */
  class PS2KeyboardServer {
    public:
      /**
       * @brief Creates a new @ref PS2KeyboardServer instance.
       */
      explicit PS2KeyboardServer() = default;

      /**
       * @brief Initializes the driver, registers the device, signals
       *        readiness, and enters the interrupt-driven event loop. Does
       *        not return under normal operation.
       */
      void Start();

    private:
      /**
       * @brief The PS/2 keyboard hardware driver.
       */
      PS2KeyboardDriver _driver;

      /**
       * @brief IPC send handle to the input server.
       */
      KernelIPC::IPCPortResourceID _inputPortResourceID
        = static_cast<KernelIPC::IPCPortResourceID>(-1);

      /**
       * @brief Sends an input event to the input server via IPC.
       * @param event The input event to report.
       */
      void _reportEvent(const ::Quantum::Input::InputEvent& event);
  };
}
