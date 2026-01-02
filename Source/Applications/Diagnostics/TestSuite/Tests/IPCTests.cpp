/**
 * @file Applications/Diagnostics/TestSuite/Tests/IPCTests.cpp
 * @brief IPC tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Handle.hpp>
#include <ABI/IPC.hpp>
#include <Bytes.hpp>

#include "Testing.hpp"
#include "Tests/IPCTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  using ::Quantum::CopyBytes;
  using ABI::Handle;
  using ABI::IPC;

  struct LoopbackPayload {
    UInt32 tag;
    UInt32 value;
  };

  bool IPCTests::TestLoopback() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      TEST_ASSERT(false, "ipc port create failed");

      return false;
    }

    IPC::Handle portHandle = IPC::OpenPort(
      portId,
      static_cast<UInt32>(IPC::Right::Send) | static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (portHandle == 0) {
      IPC::DestroyPort(portId);

      TEST_ASSERT(false, "ipc port handle open failed");

      return false;
    }

    LoopbackPayload payload { 0x1ACB00D5, 0x1234 };
    IPC::Message msg {};

    msg.length = sizeof(LoopbackPayload);
    CopyBytes(msg.payload, &payload, sizeof(LoopbackPayload));

    if (IPC::Send(portHandle, msg) != 0) {
      TEST_ASSERT(false, "ipc send failed");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    IPC::Message reply {};

    if (IPC::Receive(portHandle, reply) != 0) {
      TEST_ASSERT(false, "ipc receive failed");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    if (reply.length != sizeof(LoopbackPayload)) {
      TEST_ASSERT(false, "ipc reply length mismatch");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    LoopbackPayload received {};
    CopyBytes(&received, reply.payload, sizeof(LoopbackPayload));

    bool ok = received.tag == payload.tag && received.value == payload.value;
    TEST_ASSERT(ok, "ipc reply mismatch");

    IPC::DestroyPort(portHandle);
    IPC::CloseHandle(portHandle);

    return ok;
  }

  bool IPCTests::TestHandleTransfer() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      TEST_ASSERT(false, "ipc transfer port create failed");

      return false;
    }

    IPC::Handle portHandle = IPC::OpenPort(
      portId,
      static_cast<UInt32>(IPC::Right::Send) | static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (portHandle == 0) {
      IPC::DestroyPort(portId);

      TEST_ASSERT(false, "ipc transfer port handle open failed");

      return false;
    }

    UInt32 targetPortId = IPC::CreatePort();

    if (targetPortId == 0) {
      TEST_ASSERT(false, "ipc transfer target port create failed");

      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    IPC::Handle targetHandle = IPC::OpenPort(
      targetPortId,
      static_cast<UInt32>(IPC::Right::Send) | static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (targetHandle == 0) {
      TEST_ASSERT(false, "ipc transfer target port handle open failed");

      IPC::DestroyPort(targetPortId);
      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    if (IPC::SendHandle(portHandle, targetHandle) != 0) {
      TEST_ASSERT(false, "ipc send handle failed");

      IPC::CloseHandle(targetHandle);
      IPC::DestroyPort(targetPortId);
      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    IPC::Message reply {};

    if (IPC::Receive(portHandle, reply) != 0) {
      TEST_ASSERT(false, "ipc handle receive failed");

      IPC::CloseHandle(targetHandle);
      IPC::DestroyPort(targetPortId);
      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    IPC::Handle receivedHandle = 0;

    if (!IPC::TryGetHandleMessage(reply, receivedHandle)) {
      TEST_ASSERT(false, "ipc handle message missing");

      IPC::CloseHandle(targetHandle);
      IPC::DestroyPort(targetPortId);
      IPC::DestroyPort(portHandle);
      IPC::CloseHandle(portHandle);

      return false;
    }

    Handle::Info info {};
    bool ok = Handle::Query(receivedHandle, info) == 0;
    TEST_ASSERT(ok, "ipc handle query failed");

    if (receivedHandle != 0) {
      Handle::Close(receivedHandle);
    }

    IPC::CloseHandle(targetHandle);
    IPC::DestroyPort(targetPortId);
    IPC::DestroyPort(portHandle);
    IPC::CloseHandle(portHandle);

    return ok;
  }

  bool IPCTests::TestReceiveTimeout() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      TEST_ASSERT(false, "ipc timeout port create failed");

      return false;
    }

    IPC::Handle portHandle = IPC::OpenPort(
      portId,
      static_cast<UInt32>(IPC::Right::Receive) | static_cast<UInt32>(IPC::Right::Manage)
    );

    if (portHandle == 0) {
      IPC::DestroyPort(portId);

      TEST_ASSERT(false, "ipc timeout port handle open failed");

      return false;
    }

    IPC::Message msg {};
    bool timedOut = IPC::ReceiveTimeout(portHandle, msg, 1) != 0;

    TEST_ASSERT(timedOut, "ipc receive timeout expected");

    IPC::DestroyPort(portHandle);
    IPC::CloseHandle(portHandle);

    return timedOut;
  }

  void IPCTests::RegisterTests() {
    Testing::Register("IPC loopback", TestLoopback);
    Testing::Register("IPC handle transfer", TestHandleTransfer);
    Testing::Register("IPC receive timeout", TestReceiveTimeout);
  }
}

