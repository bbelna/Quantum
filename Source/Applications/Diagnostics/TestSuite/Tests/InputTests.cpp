/**
 * @file Applications/Diagnostics/TestSuite/Tests/InputTests.cpp
 * @brief Input device tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/InputDevices.hpp>
#include <ABI/Input.hpp>
#include <ABI/IPC.hpp>
#include <Bytes.hpp>

#include "Testing.hpp"
#include "Tests/InputTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  using ::Quantum::CopyBytes;
  using ABI::Console;
  using ABI::Devices::InputDevices;
  using ABI::Input;
  using ABI::IPC;

  void InputTests::LogSkip(CString reason) {
    if (_skipLogged) {
      return;
    }

    Console::Write("Input tests skipped (");
    Console::Write(reason ? reason : "unknown");
    Console::WriteLine(")");

    _skipLogged = true;
  }

  bool InputTests::TestKeyboardPresent() {
    UInt32 count = InputDevices::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      InputDevices::Info info {};

      if (InputDevices::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type != InputDevices::Type::Keyboard) {
        continue;
      }

      if ((info.flags & static_cast<UInt32>(InputDevices::Flag::Ready)) == 0) {
        continue;
      }

      return true;
    }

    LogSkip("keyboard not found");

    return true;
  }

  bool InputTests::TestKeyboardEvent() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      TEST_ASSERT(false, "input port create failed");

      return false;
    }

    IPC::Handle portHandle = IPC::OpenPort(
      portId,
      static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (portHandle == 0) {
      IPC::DestroyPort(portId);

      TEST_ASSERT(false, "input port handle open failed");

      return false;
    }

    if (Input::Subscribe(portId) != 0) {
      TEST_ASSERT(false, "input subscribe failed");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    Console::WriteLine("Press any key for input test...");

    bool received = false;
    Input::EventMessage payload {};

    for (;;) {
      IPC::Message msg {};

      if (IPC::Receive(portHandle, msg) != 0) {
        continue;
      }

      if (msg.length < sizeof(Input::EventMessage)) {
        continue;
      }

      CopyBytes(&payload, msg.payload, sizeof(Input::EventMessage));

      if (payload.op == Input::Operation::Event) {
        received = true;

        break;
      }
    }

    Input::Unsubscribe(portId);
    IPC::DestroyPort(portHandle);
    IPC::CloseHandle(portHandle);

    if (!received) {
      TEST_ASSERT(false, "no input event received");

      return false;
    }

    return true;
  }

  bool InputTests::TestSubscribeTimeout() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      TEST_ASSERT(false, "input timeout port create failed");

      return false;
    }

    IPC::Handle portHandle = IPC::OpenPort(
      portId,
      static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (portHandle == 0) {
      IPC::DestroyPort(portId);

      TEST_ASSERT(false, "input timeout port handle open failed");

      return false;
    }

    if (Input::Subscribe(portId, 10) != 0) {
      TEST_ASSERT(false, "input subscribe timeout failed");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    if (Input::Unsubscribe(portId, 10) != 0) {
      TEST_ASSERT(false, "input unsubscribe timeout failed");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    IPC::DestroyPort(portId);
    IPC::CloseHandle(portHandle);

    return true;
  }

  bool InputTests::TestReadTimeout() {
    UInt32 count = InputDevices::GetCount();
    UInt32 keyboardId = 0;

    for (UInt32 i = 1; i <= count; ++i) {
      InputDevices::Info info {};

      if (InputDevices::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type != InputDevices::Type::Keyboard) {
        continue;
      }

      if ((info.flags & static_cast<UInt32>(InputDevices::Flag::Ready)) == 0) {
        continue;
      }

      keyboardId = info.id;
      break;
    }

    if (keyboardId == 0) {
      LogSkip("keyboard not found");

      return true;
    }

    InputDevices::Event event {};
    UInt32 status = InputDevices::ReadEvent(keyboardId, event, 1);

    if (status == 0) {
      bool ok = event.deviceId == keyboardId;

      TEST_ASSERT(ok, "input timeout returned wrong device id");

      return ok;
    }

    return true;
  }

  void InputTests::RegisterTests() {
    Testing::Register("Input keyboard present", TestKeyboardPresent);
    Testing::Register("Input keyboard event", TestKeyboardEvent);
    Testing::Register("Input subscribe timeout", TestSubscribeTimeout);
    Testing::Register("Input read timeout", TestReadTimeout);
  }
}


