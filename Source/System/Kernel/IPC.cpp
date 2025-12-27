/**
 * @file System/Kernel/IPC.cpp
 * @brief Inter-process communication (IPC) handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "IPC.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel {
  IPC::Port* IPC::FindPort(UInt32 id) {
    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (_ports[i].used && _ports[i].id == id) {
        return &_ports[i];
      }
    }

    return nullptr;
  }

  void IPC::CopyPayload(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

  UInt32 IPC::CreatePort() {
    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (!_ports[i].used) {
        _ports[i].used = true;
        _ports[i].id = _nextPortId++;
        _ports[i].ownerTaskId = Task::GetCurrentId();
        _ports[i].head = 0;
        _ports[i].tail = 0;
        _ports[i].count = 0;

        return _ports[i].id;
      }
    }

    return 0;
  }

  bool IPC::Send(
    UInt32 portId,
    UInt32 senderId,
    const void* buffer,
    UInt32 length
  ) {
    if (!buffer || length == 0 || length > maxPayloadBytes) {
      return false;
    }

    Port* port = FindPort(portId);

    if (!port) {
      return false;
    }

    // block (cooperatively) if queue full
    while (port->count >= maxQueueDepth) {
      Task::Yield();
    }

    Message& msg = port->queue[port->tail];

    msg.senderId = senderId;
    msg.length = length;

    CopyPayload(msg.data, buffer, length);

    port->tail = (port->tail + 1) % maxQueueDepth;
    ++port->count;

    return true;
  }

  bool IPC::Receive(
    UInt32 portId,
    UInt32& outSenderId,
    void* outBuffer,
    UInt32 bufferCapacity,
    UInt32& outLength
  ) {
    if (!outBuffer || bufferCapacity == 0) {
      return false;
    }

    Port* port = FindPort(portId);

    if (!port) {
      return false;
    }

    while (port->count == 0) {
      Task::Yield();
    }

    Message& msg = port->queue[port->head];

    outSenderId = msg.senderId;
    outLength = msg.length;

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyPayload(outBuffer, msg.data, toCopy);

    port->head = (port->head + 1) % maxQueueDepth;
    --port->count;

    return true;
  }

  bool IPC::TryReceive(
    UInt32 portId,
    UInt32& outSenderId,
    void* outBuffer,
    UInt32 bufferCapacity,
    UInt32& outLength
  ) {
    if (!outBuffer || bufferCapacity == 0) {
      return false;
    }

    Port* port = FindPort(portId);

    if (!port || port->count == 0) {
      return false;
    }

    Message& msg = port->queue[port->head];

    outSenderId = msg.senderId;
    outLength = msg.length;

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyPayload(outBuffer, msg.data, toCopy);

    port->head = (port->head + 1) % maxQueueDepth;
    --port->count;

    return true;
  }

  bool IPC::DestroyPort(UInt32 portId) {
    Port* port = FindPort(portId);

    if (!port) {
      return false;
    }

    port->used = false;
    port->id = 0;
    port->ownerTaskId = 0;
    port->head = 0;
    port->tail = 0;
    port->count = 0;

    return true;
  }

  bool IPC::GetPortOwner(UInt32 portId, UInt32& outOwnerId) {
    Port* port = FindPort(portId);

    if (!port) {
      return false;
    }

    outOwnerId = port->ownerTaskId;

    return true;
  }
}
