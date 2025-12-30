/**
 * @file System/Coordinator/Devices.cpp
 * @brief Coordinator device handle broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/Devices/InputDevices.hpp>
#include <ABI/Devices/DeviceBroker.hpp>
#include <ABI/Handle.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "Devices.hpp"

namespace Quantum::System::Coordinator {
  using ABI::Console;
  using ABI::Devices::BlockDevices;
  using ABI::Devices::InputDevices;
  using ABI::Devices::DeviceBroker;
  using ABI::IPC;
  using ABI::Task;

  void Devices::Initialize() {
    if (_portId != 0) {
      return;
    }

    _portId = IPC::CreatePort();

    if (_portId == 0) {
      Console::WriteLine("Coordinator: failed to create devices port");
      return;
    }

    if (_portId != IPC::Ports::Devices) {
      Console::WriteLine("Coordinator: devices port id mismatch");
    }

    _portHandle = IPC::OpenPort(
      _portId,
      IPC::RightReceive | IPC::RightManage
    );

    if (_portHandle == 0) {
      Console::WriteLine("Coordinator: failed to open devices port handle");
    }
  }

  void Devices::StorePendingReply(UInt32 senderId, UInt32 handle) {
    if (senderId == 0 || handle == 0) {
      return;
    }

    for (UInt32 i = 0; i < _maxPendingReplies; ++i) {
      if (_pendingReplies[i].inUse && _pendingReplies[i].senderId == senderId) {
        if (_pendingReplies[i].handle != 0) {
          IPC::CloseHandle(_pendingReplies[i].handle);
        }

        _pendingReplies[i].handle = handle;

        return;
      }
    }

    for (UInt32 i = 0; i < _maxPendingReplies; ++i) {
      if (!_pendingReplies[i].inUse) {
        _pendingReplies[i].inUse = true;
        _pendingReplies[i].senderId = senderId;
        _pendingReplies[i].handle = handle;

        return;
      }
    }

    IPC::CloseHandle(handle);
  }

  UInt32 Devices::TakePendingReply(UInt32 senderId) {
    if (senderId == 0) {
      return 0;
    }

    for (UInt32 i = 0; i < _maxPendingReplies; ++i) {
      if (_pendingReplies[i].inUse && _pendingReplies[i].senderId == senderId) {
        UInt32 handle = _pendingReplies[i].handle;

        _pendingReplies[i].inUse = false;
        _pendingReplies[i].senderId = 0;
        _pendingReplies[i].handle = 0;

        return handle;
      }
    }

    return 0;
  }

  void Devices::ProcessPending() {
    if (_portId == 0) {
      return;
    }

    UInt32 receiveId = _portHandle != 0 ? _portHandle : _portId;

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(receiveId, msg) != 0) {
        break;
      }

      IPC::Handle transferHandle = 0;

      if (IPC::TryGetHandleMessage(msg, transferHandle)) {
        StorePendingReply(msg.senderId, transferHandle);

        continue;
      }

      if (msg.length < sizeof(DeviceBroker::Request)) {
        continue;
      }

      DeviceBroker::Request request {};
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      IPC::Handle replyHandle = 0;

      if (request.replyPortId != 0) {
        replyHandle = IPC::OpenPort(request.replyPortId, IPC::RightSend);
      } else {
        replyHandle = TakePendingReply(msg.senderId);
      }

      if (replyHandle == 0) {
        continue;
      }

      UInt32 status = 1;

      if (request.op == static_cast<UInt32>(DeviceBroker::Operation::OpenBlock)) {
        BlockDevices::Handle handle = BlockDevices::Open(
          request.deviceId,
          request.rights
        );

        if (handle != 0) {
          IPC::SendHandle(replyHandle, handle, request.rights);
          ABI::Handle::Close(handle);
          status = 0;
        }
      } else if (
        request.op == static_cast<UInt32>(DeviceBroker::Operation::OpenInput)
      ) {
        InputDevices::Handle handle = InputDevices::Open(
          request.deviceId,
          request.rights
        );

        if (handle != 0) {
          IPC::SendHandle(replyHandle, handle, request.rights);
          ABI::Handle::Close(handle);
          status = 0;
        }
      }

      IPC::Message reply {};

      reply.length = sizeof(status);
      for (UInt32 i = 0; i < sizeof(status); ++i) {
        reply.payload[i] = reinterpret_cast<UInt8*>(&status)[i];
      }

      IPC::Send(replyHandle, reply);
      IPC::CloseHandle(replyHandle);
    }

    Task::Yield();
  }
}
