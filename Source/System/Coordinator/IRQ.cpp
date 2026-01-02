/**
 * @file System/Coordinator/IRQ.cpp
 * @brief Coordinator IRQ routing.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Handle.hpp>
#include <ABI/IPC.hpp>
#include <ABI/IRQ.hpp>
#include <ABI/Task.hpp>

#include "IRQ.hpp"

namespace Quantum::System::Coordinator {
  using ABI::Console;
  using ABI::IPC;
  using ABI::Task;

  void IRQ::Initialize() {
    if (_portId != 0) {
      return;
    }

    _portId = IPC::CreatePort();

    if (_portId == 0) {
      Console::WriteLine("Coordinator: failed to create IRQ port");

      return;
    }

    if (_portId != static_cast<UInt32>(IPC::Ports::IRQ)) {
      Console::WriteLine("Coordinator: IRQ port id mismatch");
    }

    _portHandle = IPC::OpenPort(
      _portId,
      static_cast<UInt32>(IPC::Right::Receive)
        | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (_portHandle == 0) {
      Console::WriteLine("Coordinator: failed to open IRQ port handle");
    }
  }

  void IRQ::StorePendingReply(UInt32 senderId, UInt32 handle) {
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

  UInt32 IRQ::TakePendingReply(UInt32 senderId) {
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

  void IRQ::ProcessPending() {
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

      if (msg.length < sizeof(ABI::IRQ::Message)) {
        continue;
      }

      ABI::IRQ::Message request {};
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      if (request.op != ABI::IRQ::Operation::Register) {
        continue;
      }

      IPC::Handle replyHandle = 0;

      if (request.replyPortId != 0) {
        replyHandle = IPC::OpenPort(
          request.replyPortId,
          static_cast<UInt32>(IPC::Right::Send)
        );
      } else {
        replyHandle = TakePendingReply(msg.senderId);
      }

      ABI::IRQ::Handle irqHandle = ABI::IRQ::Open(
        request.irq,
        static_cast<UInt32>(ABI::IRQ::Right::Register)
          | static_cast<UInt32>(ABI::IRQ::Right::Unregister)
          | static_cast<UInt32>(ABI::IRQ::Right::Enable)
          | static_cast<UInt32>(ABI::IRQ::Right::Disable)
      );
      UInt32 status = 1;

      if (irqHandle != 0) {
        status = Register(irqHandle, request.portId);

        if (status == 0 && replyHandle != 0) {
          UInt32 rights = static_cast<UInt32>(ABI::IRQ::Right::Register)
            | static_cast<UInt32>(ABI::IRQ::Right::Unregister)
            | static_cast<UInt32>(ABI::IRQ::Right::Enable)
            | static_cast<UInt32>(ABI::IRQ::Right::Disable);

          IPC::SendHandle(replyHandle, irqHandle, rights);
        }

        ABI::Handle::Close(irqHandle);
      }

      if (replyHandle != 0) {
        UInt32 payload = status;
        IPC::Message reply {};

        reply.length = sizeof(payload);

        for (UInt32 i = 0; i < sizeof(payload); ++i) {
          reply.payload[i] = reinterpret_cast<UInt8*>(&payload)[i];
        }

        IPC::Send(replyHandle, reply);
        IPC::CloseHandle(replyHandle);
      }
    }

    Task::Yield();
  }
}


