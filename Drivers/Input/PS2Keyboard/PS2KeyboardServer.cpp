/**
 * @file Drivers/Input/PS2Keyboard/PS2KeyboardServer.cpp
 * @brief Implements @ref @QDrvs::Input::PS2Keyboard::PS2KeyboardServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PS2KeyboardServer.hpp"

namespace Quantum::Drivers::Input::PS2Keyboard {
  void PS2KeyboardServer::Start() {
    ProcessID pid = Process::GetID();

    // initialize the hardware driver (permissions + IRQ claim)
    if (!_driver.Initialize()) return;

    // register the device with the device server
    Device ps2Device {
      PS2KeyboardDriver::DeviceIdentifier,
      "PS2Keyboard",
      "PS/2 Keyboard",
      ToDeviceCategoryID(DeviceCategoryType::Input),
      DeviceState::Active,
      pid,
      0,
      DeviceBus::ISA,
      0,
      {}
    };

    SetISAAddress(
      ps2Device,
      { PS2KeyboardDriver::DataPort, PS2KeyboardDriver::IRQLine }
    );

    Quantum::Clients::DeviceClient deviceClient;

    deviceClient.Add(ps2Device);

    // wait for the input server, then open a persistent send handle
    KernelClient kernelClient;
    kernelClient.WaitForIPCPort(InputPortID);

    _inputPortResourceID = KernelIPC::Open(
      InputPortID,
      IPCPortRights::Send
    );

    // signal readiness after all initialization is complete
    StartupClient startup;
    startup.Ready();

    Threads::SetPriority(Threads::Priority::InputDriver);

    KernelLog::Write(LogLevel::Trace, "Driver ready, entering IRQ loop");

    // main interrupt-driven loop
    for (;;) {
      Interrupts::Wait(_driver.GetIRQHandle());

      // read the scancode from the data port
      UInt8 rawScancode = PortIO::In8(PS2KeyboardDriver::DataPort);

      InputEvent event;

      if (_driver.ProcessScancode(rawScancode, &event)) {
        _reportEvent(event);
      }
    }
  }

  void PS2KeyboardServer::_reportEvent(const InputEvent& event) {
    InputReportEventRequest request = {};

    request.ABIVersion = InputABIVersion;
    request.Operation = InputOperation::ReportEvent;
    request.Event = event;

    KernelIPC::Send(
      _inputPortResourceID,
      static_cast<const void*>(&request),
      sizeof(InputReportEventRequest)
    );
  }
}

/**
 * @brief Entry point for @ref @QDrvs::Input::PS2Keyboard.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a device driver and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 */
int Main() {
  PS2KeyboardServer server;

  server.Start();

  return -1;
}
