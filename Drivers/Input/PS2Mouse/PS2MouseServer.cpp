/**
 * @file Drivers/Input/PS2Mouse/PS2MouseServer.cpp
 * @brief Implements @ref @QDrvs::Input::PS2Mouse::PS2MouseServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PS2MouseServer.hpp"

namespace Quantum::Drivers::Input::PS2Mouse {
  void PS2MouseServer::Start() {
    ProcessID pid = Process::GetID();

    if (!_driver.Initialize()) return;

    // register the device with the device server
    Device mouseDevice {
      PS2MouseDriver::DeviceIdentifier,
      "PS2Mouse",
      "PS/2 Mouse",
      ToDeviceCategoryID(DeviceCategoryType::Input),
      DeviceState::Active,
      pid,
      0,
      DeviceBus::ISA,
      0,
      {}
    };

    SetISAAddress(
      mouseDevice,
      { PS2MouseDriver::DataPort, PS2MouseDriver::IRQLine }
    );

    Quantum::Clients::DeviceClient deviceClient;

    deviceClient.Add(mouseDevice);

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
    static constexpr Size MaxEventsPerPacket = 5;

    InputEvent events[MaxEventsPerPacket];

    for (;;) {
      Interrupts::Wait(_driver.GetIRQHandle());

      // drain all available mouse bytes from the PS/2 controller;
      // the 8259A PIC is edge-triggered and can miss rapid LOW->HIGH
      // transitions when the controller immediately loads the next
      // queued byte after a read, so we must not rely on one IRQ
      // per byte
      for (;;) {
        UInt8 controllerStatus = PortIO::In8(
          PS2MouseDriver::CommandPort
        );

        // bit 0: output buffer full (data available)
        // bit 5: data is from the auxiliary (mouse) device
        if ((controllerStatus & 0x21) != 0x21) break;

        UInt8 data = PortIO::In8(PS2MouseDriver::DataPort);

        Size eventCount = _driver.ProcessByte(
          data, events, MaxEventsPerPacket
        );

        for (Size eventIndex = 0; eventIndex < eventCount; eventIndex++) {
          _reportEvent(events[eventIndex]);
        }
      }
    }
  }

  void PS2MouseServer::_reportEvent(const InputEvent& event) {
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
 * @brief Entry point for @ref @QDrvs::Input::PS2Mouse.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a device driver and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 */
int Main() {
  PS2MouseServer server;

  server.Start();

  return -1;
}
