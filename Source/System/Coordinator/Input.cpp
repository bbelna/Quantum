/**
 * @file System/Coordinator/Input.cpp
 * @brief Coordinator input broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/InputDevices.hpp>
#include <ABI/Input.hpp>
#include <ABI/IPC.hpp>
#include <Bytes.hpp>
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

    if (_portId != static_cast<UInt32>(IPC::Ports::Input)) {
      Console::WriteLine("Coordinator: input port id mismatch");
    }

    _portHandle = IPC::OpenPort(
      _portId,
      static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (_portHandle == 0) {
      Console::WriteLine("Coordinator: failed to open input port handle");
    }
  }

  ABI::Input::Status Input::AddSubscriber(UInt32 portId) {
    if (portId == 0) {
      return ABI::Input::Status::Invalid;
    }

    for (UInt32 i = 0; i < _maxSubscribers; ++i) {
      if (_subscriberPorts[i] == portId) {
        return ABI::Input::Status::Ok;
      }
    }

    for (UInt32 i = 0; i < _maxSubscribers; ++i) {
      if (_subscriberPorts[i] == 0) {
        _subscriberPorts[i] = portId;

        return ABI::Input::Status::Ok;
      }
    }

    return ABI::Input::Status::Full;
  }

  ABI::Input::Status Input::RemoveSubscriber(UInt32 portId) {
    if (portId == 0) {
      return ABI::Input::Status::Invalid;
    }

    for (UInt32 i = 0; i < _maxSubscribers; ++i) {
      if (_subscriberPorts[i] == portId) {
        _subscriberPorts[i] = 0;

        return ABI::Input::Status::Ok;
      }
    }

    return ABI::Input::Status::NotFound;
  }

  void Input::ProcessPending() {
    if (_portId == 0) {
      return;
    }

    UInt32 receiveId = _portHandle != 0 ? _portHandle : _portId;

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(receiveId, msg) != 0) {
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

      ABI::Input::Status status = ABI::Input::Status::Invalid;

      if (request.op == ABI::Input::Operation::Subscribe) {
        status = AddSubscriber(request.portId);
      } else if (request.op == ABI::Input::Operation::Unsubscribe) {
        status = RemoveSubscriber(request.portId);
      } else {
        continue;
      }

      UInt32 statusValue = static_cast<UInt32>(status);
      IPC::Message reply {};

      reply.length = sizeof(statusValue);

      ::Quantum::CopyBytes(reply.payload, &statusValue, sizeof(statusValue));

      IPC::Handle replyHandle = IPC::OpenPort(
        request.portId,
        static_cast<UInt32>(IPC::Right::Send)
      );

      if (replyHandle == 0) {
        continue;
      }

      IPC::Send(replyHandle, reply);
      IPC::CloseHandle(replyHandle);
    }

    UInt32 count = InputDevices::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      for (;;) {
        InputDevices::Event event {};

        if (InputDevices::ReadEvent(i, event) != 0) {
          break;
        }

        ABI::Input::EventMessage payload {};

        payload.op = ABI::Input::Operation::Event;
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

          IPC::Handle subscriberHandle = IPC::OpenPort(
            portId,
            static_cast<UInt32>(IPC::Right::Send)
          );

          if (subscriberHandle == 0) {
            continue;
          }

          IPC::Send(subscriberHandle, message);
          IPC::CloseHandle(subscriberHandle);
        }
      }
    }

    Task::Yield();
  }
}

