/**
 * @file Drivers/Input/PS2Mouse/PS2MouseDriver.cpp
 * @brief Implements @ref @QDrvs::Input::PS2Mouse::PS2MouseDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PS2MouseDriver.hpp"

namespace Quantum::Drivers::Input::PS2Mouse {
  bool PS2MouseDriver::Initialize() {
    // request Port I/O permission (needed to access 0x60/0x64)
    if (!Process::RequestPermission(ProcessPermissions::PortIO)) {
      KernelLog::Write(
        LogLevel::Error,
        "Failed to acquire Port I/O permission"
      );

      return false;
    }

    // request interrupt permission (needed to claim IRQ 12)
    if (!Process::RequestPermission(ProcessPermissions::Interrupts)) {
      KernelLog::Write(
        LogLevel::Error,
        "Failed to acquire interrupt permission"
      );

      return false;
    }

    // initialize the PS/2 mouse hardware
    _initializeMouse();

    // claim IRQ 12
    _irqHandle = Interrupts::Claim(IRQLine);

    if (_irqHandle == static_cast<ResourceID>(-1)) {
      KernelLog::Write(
        LogLevel::Error,
        "Failed to claim IRQ %u",
        static_cast<UInt32>(IRQLine)
      );

      return false;
    }

    KernelLog::Write(
      LogLevel::Trace,
      "Claimed IRQ %u (handle %u)",
      static_cast<UInt32>(IRQLine),
      _irqHandle
    );

    return true;
  }

  ResourceID PS2MouseDriver::GetIRQHandle() const {
    return _irqHandle;
  }

  Size PS2MouseDriver::ProcessByte(
    UInt8 data,
    InputEvent* outEvents,
    Size maxEvents
  ) {
    // first byte of a packet must have bit 3 set
    if (_packetIndex == 0 && !(data & 0x08)) return 0;

    _packetBuffer[_packetIndex++] = data;

    if (_packetIndex == _packetSize) {
      _packetIndex = 0;

      return _processPacket(outEvents, maxEvents);
    }

    return 0;
  }

  void PS2MouseDriver::_initializeMouse() {
    _waitForInput();

    // enable the auxiliary (mouse) PS/2 port
    PortIO::Out8(CommandPort, 0xA8);

    _waitForInput();

    // read the controller configuration byte
    PortIO::Out8(CommandPort, 0x20);

    _waitForOutput();

    UInt8 config = PortIO::In8(DataPort);

    // enable IRQ12 (bit 1) and clear disable-aux-clock (bit 5)
    config |= 0x02;
    config &= ~0x20;

    _waitForInput();

    // write the modified configuration byte
    PortIO::Out8(CommandPort, 0x60);

    _waitForInput();

    // write the new config value back to the controller
    PortIO::Out8(DataPort, config);

    // send "set defaults" command to mouse
    _sendMouseCommand(0xF6);

    // attempt IntelliMouse detection (scroll wheel support)
    // magic sample rate sequence: 200, 100, 80
    _setSampleRate(200);
    _setSampleRate(100);
    _setSampleRate(80);

    UInt8 deviceID = _getDeviceID();

    if (deviceID == 3) {
      _hasScrollWheel = true;
      _packetSize = 4;
    }

    // send "enable data reporting" command to mouse
    _sendMouseCommand(0xF4);
  }

  bool PS2MouseDriver::_waitForInput() {
    for (int i = 0; i < 100000; ++i) {
      if (!(PortIO::In8(CommandPort) & 0x02)) return true;
    }

    return false;
  }

  bool PS2MouseDriver::_waitForOutput() {
    for (int i = 0; i < 100000; ++i) {
      if (PortIO::In8(CommandPort) & 0x01) return true;
    }

    return false;
  }

  void PS2MouseDriver::_sendMouseCommand(UInt8 command) {
    if (!_waitForInput()) {
      KernelLog::Write(
        LogLevel::Warning,
        "Input timeout before 0xD4 prefix"
      );

      return;
    }

    // prefix: next byte goes to the auxiliary (mouse) device
    PortIO::Out8(CommandPort, 0xD4);

    if (!_waitForInput()) {
      KernelLog::Write(
        LogLevel::Warning,
        "Input timeout before command 0x%x",
        command
      );

      return;
    }

    // send the command byte
    PortIO::Out8(DataPort, command);

    if (!_waitForOutput()) {
      KernelLog::Write(
        LogLevel::Warning,
        "Output timeout after command 0x%x",
        command
      );

      return;
    }

    // read acknowledgment
    PortIO::In8(DataPort);
  }

  void PS2MouseDriver::_setSampleRate(UInt8 rate) {
    _sendMouseCommand(0xF3);
    _sendMouseCommand(rate);
  }

  UInt8 PS2MouseDriver::_getDeviceID() {
    _sendMouseCommand(0xF2);

    if (!_waitForOutput()) {
      KernelLog::Write(
        LogLevel::Warning,
        "Output timeout reading device ID"
      );

      return 0;
    }

    return PortIO::In8(DataPort);
  }

  Size PS2MouseDriver::_processPacket(
    InputEvent* outEvents,
    Size maxEvents
  ) {
    UInt8 status = _packetBuffer[0];

    // discard packets with overflow bits set
    if (status & 0xC0) return 0;

    Size eventCount = 0;

    // extract and sign-extend deltas
    Int16 deltaX = static_cast<Int16>(_packetBuffer[1]);
    Int16 deltaY = static_cast<Int16>(_packetBuffer[2]);

    if (status & 0x10) deltaX |= static_cast<Int16>(0xFF00);
    if (status & 0x20) deltaY |= static_cast<Int16>(0xFF00);

    // invert Y axis (PS/2 positive = up, screen Y increases downward)
    deltaY = static_cast<Int16>(-deltaY);

    UInt8 buttons = status & 0x07;

    // report scroll wheel movement
    if (_hasScrollWheel) {
      Int8 deltaZ = static_cast<Int8>(_packetBuffer[3]);

      if (deltaZ != 0 && eventCount < maxEvents) {
        InputEvent& event = outEvents[eventCount];

        event = {};
        event.Type = InputEventType::MouseScroll;
        event.SourceDeviceID = DeviceIdentifier;
        event.DeltaZ = deltaZ;
        event.Buttons = static_cast<MouseButton>(buttons);

        eventCount++;
      }
    }

    // report mouse movement
    if ((deltaX != 0 || deltaY != 0) && eventCount < maxEvents) {
      InputEvent& event = outEvents[eventCount];

      event = {};
      event.Type = InputEventType::MouseMove;
      event.SourceDeviceID = DeviceIdentifier;
      event.DeltaX = deltaX;
      event.DeltaY = deltaY;
      event.Buttons = static_cast<MouseButton>(buttons);

      eventCount++;
    }

    // report button state changes
    UInt8 changed = buttons ^ _lastButtons;

    if (changed) {
      for (UInt8 bit = 0; bit < 3; ++bit) {
        if ((changed & (1 << bit)) && eventCount < maxEvents) {
          InputEvent& event = outEvents[eventCount];

          event = {};
          event.Type = (buttons & (1 << bit))
            ? InputEventType::MouseButtonDown
            : InputEventType::MouseButtonUp;
          event.SourceDeviceID = DeviceIdentifier;
          event.Buttons = static_cast<MouseButton>(buttons);

          eventCount++;
        }
      }

      _lastButtons = buttons;
    }

    return eventCount;
  }
}
