/**
 * @file System/Coordinator/IRQ.cpp
 * @brief Coordinator IRQ routing.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
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

    if (_portId != IPC::Ports::IRQ) {
      Console::WriteLine("Coordinator: IRQ port id mismatch");
    }
  }

  void IRQ::ProcessPending() {
    if (_portId == 0) {
      return;
    }

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(_portId, msg) != 0) {
        break;
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

      if (request.op != static_cast<UInt32>(ABI::IRQ::Operation::Register)) {
        continue;
      }

      UInt32 status = Register(request.irq, request.portId);

      if (request.replyPortId != 0) {
        UInt32 payload = status;
        IPC::Message reply {};

        reply.length = sizeof(payload);

        for (UInt32 i = 0; i < sizeof(payload); ++i) {
          reply.payload[i] = reinterpret_cast<UInt8*>(&payload)[i];
        }

        IPC::Send(request.replyPortId, reply);
      }
    }

    Task::Yield();
  }
}
