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
  using ABI::Task;
  using PS2::Controller;

  static constexpr UInt8 scancodeMap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,
    '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
  };

  static constexpr UInt8 scancodeMapShift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',':','\"','~', 0,
    '|','z','x','c','v','b','n','m','<','>','?', 0, '*', 0, ' ',
  };

  static constexpr UInt8 shiftLeftMake = 0x2A;
  static constexpr UInt8 shiftRightMake = 0x36;
  static constexpr UInt8 shiftLeftBreak = 0xAA;
  static constexpr UInt8 shiftRightBreak = 0xB6;
  static constexpr UInt8 ctrlMake = 0x1D;
  static constexpr UInt8 ctrlBreak = 0x9D;
  static constexpr UInt8 altMake = 0x38;
  static constexpr UInt8 altBreak = 0xB8;
  static constexpr UInt8 capsMake = 0x3A;
  static constexpr UInt8 capsBreak = 0xBA;

  void Driver::RegisterIRQRoute(UInt32 portId) {
    ABI::IRQ::Handle handle = 0;
    UInt32 status = ABI::IRQ::Register(_irqLine, portId, &handle);

    if (status != 0) {
      Console::WriteLine("PS/2 keyboard IRQ register failed");

      if (handle != 0) {
        ABI::Handle::Close(handle);
      }

      return;
    }

    _irqHandle = handle;

    if (_irqHandle != 0) {
      ABI::IRQ::Enable(_irqHandle);
    }
  }

  void Driver::SendReadySignal(UInt8 deviceTypeId) {
    ABI::Coordinator::ReadyMessage ready {};
    IPC::Message msg {};

    ready.deviceId = deviceTypeId;
    ready.state = 1;

    msg.length = sizeof(ready);

    CopyBytes(msg.payload, &ready, msg.length);

    IPC::Handle readyHandle = IPC::OpenPort(
      ABI::IPC::Ports::CoordinatorReady,
      IPC::RightSend
    );

    if (readyHandle == 0) {
      return;
    }

    IPC::Send(readyHandle, msg);
    IPC::CloseHandle(readyHandle);
  }

  bool Driver::IsIRQMessage(const IPC::Message& msg) {
    if (msg.length < sizeof(ABI::IRQ::Message)) {
      return false;
    }

    ABI::IRQ::Message header {};
    UInt32 copyBytes = msg.length;

    if (copyBytes > sizeof(ABI::IRQ::Message)) {
      copyBytes = sizeof(ABI::IRQ::Message);
    }

    CopyBytes(&header, msg.payload, copyBytes);

    return header.op == 0 && header.irq == _irqLine;
  }

  UInt32 Driver::BuildModifiers() {
    UInt32 mods = 0;

    if (_shiftActive) {
      mods |= InputDevices::modShift;
    }

    if (_ctrlActive) {
      mods |= InputDevices::modCtrl;
    }

    if (_altActive) {
      mods |= InputDevices::modAlt;
    }

    if (_capsLock) {
      mods |= InputDevices::modCaps;
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

    event.type = type;
    event.deviceId = _deviceId;
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

    if (scancode == shiftLeftMake || scancode == shiftRightMake) {
      _shiftActive = true;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == shiftLeftBreak || scancode == shiftRightBreak) {
      _shiftActive = false;

      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (scancode == ctrlMake) {
      _ctrlActive = true;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == ctrlBreak) {
      _ctrlActive = false;

      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (scancode == altMake) {
      _altActive = true;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == altBreak) {
      _altActive = false;

      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (scancode == capsMake) {
      _capsLock = !_capsLock;

      SendKeyEvent(code, InputDevices::EventType::KeyDown, 0, 0);

      return;
    }

    if (scancode == capsBreak) {
      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    if (code >= sizeof(scancodeMap)) {
      return;
    }

    if (isBreak) {
      SendKeyEvent(code, InputDevices::EventType::KeyUp, 0, 0);

      return;
    }

    UInt8 base = scancodeMap[code];
    UInt8 ch = base;

    if (base >= 'a' && base <= 'z') {
      bool upper = (_shiftActive ^ _capsLock);

      ch = upper ? static_cast<UInt8>(base - ('a' - 'A')) : base;
    } else if (_shiftActive) {
      ch = scancodeMapShift[code];
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
      IPC::RightReceive | IPC::RightManage
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
    info.flags = InputDevices::flagReady;
    info.deviceIndex = 0;

    _deviceId = InputDevices::Register(info);

    if (_deviceId == 0) {
      Console::WriteLine("PS/2 keyboard device registration failed");
      Task::Exit(1);
    }

    Console::WriteLine("PS/2 keyboard driver ready");

    SendReadySignal(2);

    for (;;) {
      IPC::Message msg {};

      if (IPC::Receive(portHandle, msg) != 0) {
        Task::Yield();

        continue;
      }

      if (IsIRQMessage(msg)) {
        HandleIRQ();
      }
    }
  }
}
