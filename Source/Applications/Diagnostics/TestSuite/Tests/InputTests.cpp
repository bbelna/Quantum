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
#include <ABI/Task.hpp>
#include <Bytes.hpp>

#include "Testing.hpp"
#include "Tests/InputTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  using ::Quantum::CopyBytes;
  using ABI::Console;
  using ABI::Devices::InputDevices;
  using ABI::Input;
  using ABI::IPC;
  using ABI::Task;

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

      if ((info.flags & InputDevices::flagReady) == 0) {
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
      IPC::RightReceive | IPC::RightManage
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
        Task::Yield();

        continue;
      }

      if (msg.length < sizeof(Input::EventMessage)) {
        continue;
      }

      CopyBytes(&payload, msg.payload, sizeof(Input::EventMessage));

      if (payload.op == 0) {
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

  void InputTests::RegisterTests() {
    Testing::Register("Input keyboard present", TestKeyboardPresent);
    Testing::Register("Input keyboard event", TestKeyboardEvent);
  }
}
