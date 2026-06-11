/**
 * @file Drivers/Input/PS2Mouse/PS2MouseServer.hpp
 * @brief Declares @ref @QDrvs::Input::PS2Mouse::PS2MouseServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "PS2MouseDriver.hpp"

namespace Quantum::Drivers::Input::PS2Mouse {
  /**
   * @brief Interrupt-driven event loop server for the PS/2 mouse. Owns the
   *        hardware driver, registers the device, and forwards input events
   *        to the input server via IPC.
   */
  class PS2MouseServer {
    public:
      /**
       * @brief Creates a new @ref PS2MouseServer instance.
       */
      explicit PS2MouseServer() = default;

      /**
       * @brief Initializes the hardware driver, registers the PS/2 mouse
       *        device, opens the input server IPC port, signals readiness,
       *        and enters the interrupt-driven event loop. Does not return
       *        under normal operation.
       */
      void Start();

    private:
      /**
       * @brief The PS/2 mouse hardware driver.
       */
      PS2MouseDriver _driver;

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
