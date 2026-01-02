/**
 * @file Services/Drivers/Input/PS2/Keyboard/Driver.cpp
 * @brief PS/2 keyboard driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Coordinator.hpp>
#include <ABI/Devices/InputDevices.hpp>
#include <ABI/Handle.hpp>
#include <ABI/IO.hpp>
#include <ABI/IPC.hpp>
#include <ABI/IRQ.hpp>
#include <ABI/Prelude.hpp>
#include <ABI/Task.hpp>
#include <Bytes.hpp>

#include "Controller.hpp"
#include "Driver.hpp"
#include "Prelude.hpp"

namespace Quantum::Services::Drivers::Input::PS2::Keyboard {
  using ::Quantum::CopyBytes;
  using ABI::Console;
  using ABI::Devices::InputDevices;
  using ABI::IPC;
  using ABI::IRQ;
  using ABI::Task;
  using PS2::Controller;

  void Driver::RegisterIRQRoute(UInt32 portId) {
    IRQ::Handle handle = 0;
    UInt32 status = IRQ::Register(_irqLine, portId, &handle);

    if (status != 0) {
      Console::WriteLine("PS/2 keyboard IRQ register failed");

      if (handle != 0) {
        ABI::Handle::Close(handle);
      }

      return;
    }

    _irqHandle = handle;

    if (_irqHandle != 0) {
      IRQ::Enable(_irqHandle);
    }
  }

  void Driver::SendReadySignal(UInt8 deviceTypeId) {
    ABI::Coordinator::ReadyMessage ready {};
    ready.deviceId = deviceTypeId;
    ready.state = 1;

    IPC::Message msg {};
    msg.length = sizeof(ready);

    CopyBytes(msg.payload, &ready, msg.length);

    IPC::Handle readyHandle = IPC::OpenPort(
      static_cast<UInt32>(IPC::Ports::CoordinatorReady),
      static_cast<UInt32>(IPC::Right::Send)
    );

    if (readyHandle == 0) {
      Console::WriteLine("PS/2 keyboard ready port open failed");

      return;
    }

    IPC::Send(readyHandle, msg);
    IPC::CloseHandle(readyHandle);
  }

  bool Driver::IsIRQMessage(const IPC::Message& msg) {
    if (msg.length < sizeof(IRQ::Message)) {
      return false;
    }

    IRQ::Message header {};
    UInt32 copyBytes = msg.length;

    if (copyBytes > sizeof(IRQ::Message)) {
      copyBytes = sizeof(IRQ::Message);
    }

    CopyBytes(&header, msg.payload, copyBytes);

    return header.op == IRQ::Operation::Notify && header.irq == _irqLine;
  }

  UInt32 Driver::BuildModifiers() {
    UInt32 mods = 0;

    if (_shiftActive) {
      mods |= static_cast<UInt32>(InputDevices::Modifier::Shift);
    }

    if (_ctrlActive) {
      mods |= static_cast<UInt32>(InputDevices::Modifier::Ctrl);
    }

    if (_altActive) {
      mods |= static_cast<UInt32>(InputDevices::Modifier::Alt);
    }

    if (_capsLock) {
      mods |= static_cast<UInt32>(InputDevices::Modifier::Caps);
    }

    return mods;
  }

  void Driver::SendKeyEvent(
    UInt32 keyCode,
    InputDevices::EventType type,
    UInt32 ascii,
    UInt32 unicode
  ) {
    if (_deviceId == 0) {
      return;
    }

    InputDevices::Event event {};
    UInt32 deviceToken = _deviceHandle != 0 ? _deviceHandle : _deviceId;

    event.type = type;
    event.deviceId = deviceToken;
    event.keyCode = keyCode;
    event.modifiers = BuildModifiers();
    event.ascii = ascii;
    event.unicode = unicode;

    InputDevices::PushEvent(_deviceId, event);
  }

  void Driver::HandleScancode(UInt8 scancode) {
    if (scancode == 0xE0) {
      _extendedPrefix = true;

      return;
    }

    if (_extendedPrefix) {
      _extendedPrefix = false;

      return;
    }

    bool isBreak = (scancode & 0x80) != 0;
    UInt8 code = static_cast<UInt8>(scancode & 0x7F);

    if (scancode == _shiftLeftMake || scancode == _shiftRightMake) {
      _shiftActive = true;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == _shiftLeftBreak || scancode == _shiftRightBreak) {
      _shiftActive = false;

      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (scancode == _ctrlMake) {
      _ctrlActive = true;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == _ctrlBreak) {
      _ctrlActive = false;

      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (scancode == _altMake) {
      _altActive = true;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == _altBreak) {
      _altActive = false;

      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (scancode == _capsMake) {
      _capsLock = !_capsLock;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == _capsBreak) {
      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (code >= sizeof(_scancodeMap)) {
      return;
    }

    if (isBreak) {
      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    UInt8 base = _scancodeMap[code];
    UInt8 ch = base;

    if (base >= 'a' && base <= 'z') {
      bool upper = (_shiftActive ^ _capsLock);

      ch = upper ? static_cast<UInt8>(base - ('a' - 'A')) : base;
    } else if (_shiftActive) {
      ch = _scancodeMapShift[code];
    }

    UInt32 ascii = ch != 0 ? static_cast<UInt32>(ch) : 0;
    UInt32 unicode = ascii;

    SendKeyEvent(code, InputDevices::EventType::KeyDown, ascii, unicode);
  }

  void Driver::HandleIRQ() {
    UInt8 scancode = Controller::ReadData();

    if (scancode == 0) {
      return;
    }

    HandleScancode(scancode);
  }

  void Driver::Main() {
    Console::WriteLine("PS/2 keyboard driver starting");

    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      Console::WriteLine("PS/2 keyboard failed to create IPC port");
      Task::Exit(1);
    }

    _portId = portId;

    RegisterIRQRoute(portId);

    IPC::Handle portHandle = IPC::OpenPort(
      portId,
      static_cast<UInt32>(IPC::Right::Receive)
        | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (portHandle == 0) {
      Console::WriteLine("PS/2 keyboard failed to open IPC handle");
      IPC::DestroyPort(portId);
      Task::Exit(1);
    }

    if (!Controller::Initialize()) {
      Console::WriteLine("PS/2 keyboard controller init failed");
      Task::Exit(1);
    }

    InputDevices::Info info {};

    info.id = 0;
    info.type = InputDevices::Type::Keyboard;
    info.flags = static_cast<UInt32>(InputDevices::Flag::Ready);
    info.deviceIndex = 0;

    _deviceId = InputDevices::Register(info);

    if (_deviceId == 0) {
      Console::WriteLine("PS/2 keyboard device registration failed");
      Task::Exit(1);
    }

    _deviceHandle = InputDevices::Open(
      _deviceId,
      static_cast<UInt32>(InputDevices::Right::Register)
        | static_cast<UInt32>(InputDevices::Right::Read)
        | static_cast<UInt32>(InputDevices::Right::Control)
    );

    Console::WriteLine("PS/2 keyboard driver ready");

    SendReadySignal(2);

    for (;;) {
      IPC::Message msg {};

      if (IPC::Receive(portHandle, msg) != 0) {
        continue;
      }

      if (IsIRQMessage(msg)) {
        HandleIRQ();
      }
    }
  }
}




