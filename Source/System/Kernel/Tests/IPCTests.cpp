/**
 * @file System/Kernel/Tests/IPCTests.cpp
 * @brief IPC-related kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "IPC.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "Tests/IPCTests.hpp"

namespace Quantum::System::Kernel::Tests {
  void IPCTests::SenderTask() {
    // Give the receiver a chance to block first.
    for (int i = 0; i < 4; ++i) {
      Task::Yield();
    }

    const UInt8 payload[] = { 'p', 'i', 'n', 'g' };

    _sendDone = IPC::Send(
      _portId,
      Task::GetCurrentId(),
      payload,
      static_cast<UInt32>(sizeof(payload))
    );

    Task::Exit();
  }

  void IPCTests::ReceiverTask() {
    UInt32 sender = 0;
    UInt32 length = 0;

    bool ok = IPC::Receive(
      _portId,
      sender,
      _recvBuffer,
      static_cast<UInt32>(sizeof(_recvBuffer)),
      length
    );

    _recvOk = ok;
    _recvLength = length;
    _recvDone = true;

    Task::Exit();
  }

  bool IPCTests::TestSendReceive() {
    _sendDone = false;
    _recvDone = false;
    _recvOk = false;
    _recvLength = 0;

    _portId = IPC::CreatePort();

    TEST_ASSERT(_portId != 0, "failed to create IPC port");

    Task::Create(ReceiverTask, 4096);
    Task::Create(SenderTask, 4096);

    const UInt32 maxIterations = 128;

    for (UInt32 i = 0; i < maxIterations; ++i) {
      if (_sendDone && _recvDone) {
        break;
      }

      Task::Yield();
    }

    bool ok = _sendDone && _recvDone && _recvOk;

    TEST_ASSERT(ok, "IPC send/receive did not complete");
    TEST_ASSERT(_recvLength == 4, "IPC payload length mismatch");
    TEST_ASSERT(
      _recvBuffer[0] == 'p' && _recvBuffer[1] == 'i'
        && _recvBuffer[2] == 'n' && _recvBuffer[3] == 'g',
      "IPC payload mismatch"
    );

    IPC::DestroyPort(_portId);
    _portId = 0;

    return ok;
  }

  void IPCTests::RegisterTests() {
    Testing::Register("IPC send/receive", TestSendReceive);
  }
}
