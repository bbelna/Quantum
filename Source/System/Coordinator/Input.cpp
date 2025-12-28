/**
 * @file System/Coordinator/Input.cpp
 * @brief Coordinator input broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/InputDevices.hpp>
#include <ABI/Input.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "Input.hpp"

namespace Quantum::System::Coordinator {
  using ABI::Console;
  using ABI::Devices::InputDevices;
  using ABI::IPC;
  using ABI::Task;

  void Input::Initialize() {
    if (_portId != 0) {
      return;
    }

    _portId = IPC::CreatePort();

    if (_portId == 0) {
      Console::WriteLine("Coordinator: failed to create input port");
      return;
    }

    if (_portId != IPC::Ports::Input) {
      Console::WriteLine("Coordinator: input port id mismatch");
    }
  }

  void Input::AddSubscriber(UInt32 portId) {
    if (portId == 0) {
      return;
    }

    for (UInt32 i = 0; i < _maxSubscribers; ++i) {
      if (_subscriberPorts[i] == portId) {
        return;
      }
    }

    for (UInt32 i = 0; i < _maxSubscribers; ++i) {
      if (_subscriberPorts[i] == 0) {
        _subscriberPorts[i] = portId;

        return;
      }
    }
  }

  void Input::RemoveSubscriber(UInt32 portId) {
    if (portId == 0) {
      return;
    }

    for (UInt32 i = 0; i < _maxSubscribers; ++i) {
      if (_subscriberPorts[i] == portId) {
        _subscriberPorts[i] = 0;

        return;
      }
    }
  }

  void Input::ProcessPending() {
    if (_portId == 0) {
      return;
    }

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(_portId, msg) != 0) {
        break;
      }

      if (msg.length < sizeof(ABI::Input::SubscribeMessage)) {
        continue;
      }

      ABI::Input::SubscribeMessage request {};
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      if (
        request.op
        == static_cast<UInt32>(ABI::Input::Operation::Subscribe)
      ) {
        AddSubscriber(request.portId);
      } else if (
        request.op
        == static_cast<UInt32>(ABI::Input::Operation::Unsubscribe)
      ) {
        RemoveSubscriber(request.portId);
      }
    }

    UInt32 count = InputDevices::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      for (;;) {
        InputDevices::Event event {};

        if (InputDevices::ReadEvent(i, event) != 0) {
          break;
        }

        ABI::Input::EventMessage payload {};

        payload.op = 0;
        payload.event = event;

        IPC::Message message {};

        message.length = sizeof(payload);

        for (UInt32 j = 0; j < message.length; ++j) {
          message.payload[j] = reinterpret_cast<UInt8*>(&payload)[j];
        }

        for (UInt32 j = 0; j < _maxSubscribers; ++j) {
          UInt32 portId = _subscriberPorts[j];

          if (portId == 0) {
            continue;
          }

          IPC::Send(portId, message);
        }
      }
    }

    Task::Yield();
  }
}
