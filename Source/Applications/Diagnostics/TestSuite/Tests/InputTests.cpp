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

#include "Testing.hpp"
#include "Tests/InputTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
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

  void InputTests::CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
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

    if (Input::Subscribe(portId) != 0) {
      TEST_ASSERT(false, "input subscribe failed");

      return false;
    }

    Console::WriteLine("Press any key for input test...");

    bool received = false;
    Input::EventMessage payload {};

    const UInt32 maxSpins = 200000;

    for (UInt32 i = 0; i < maxSpins; ++i) {
      IPC::Message msg {};

      if (IPC::TryReceive(portId, msg) == 0) {
        if (msg.length >= sizeof(Input::EventMessage)) {
          CopyBytes(&payload, msg.payload, sizeof(Input::EventMessage));

          if (payload.op == 0) {
            received = true;

            break;
          }
        }
      }

      if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }

    Input::Unsubscribe(portId);

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
