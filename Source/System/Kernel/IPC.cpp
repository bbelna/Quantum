/**
 * @file System/Kernel/IPC.cpp
 * @brief Inter-process communication (IPC) handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Bytes.hpp>
#include <Types.hpp>

#include "Handles.hpp"
#include "IPC.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel {
  using ::Quantum::CopyBytes;
  using Objects::IPCPortObject;
  using Objects::KernelObject;

  IPC::Port* IPC::FindPort(UInt32 id) {
    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (_ports[i].used && _ports[i].id == id) {
        return &_ports[i];
      }
    }

    return nullptr;
  }

  UInt32 IPC::CreatePort() {
    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (!_ports[i].used) {
        _ports[i].used = true;
        _ports[i].id = _nextPortId++;
        _ports[i].ownerTaskId = Task::GetCurrentId();
        _ports[i].object = new IPCPortObject(_ports[i].id);
        _ports[i].head = 0;
        _ports[i].tail = 0;
        _ports[i].count = 0;

        if (!_ports[i].object) {
          _ports[i].used = false;
          _ports[i].id = 0;
          _ports[i].ownerTaskId = 0;

          return 0;
        }

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
    msg.hasTransfer = false;
    msg.transferObject = nullptr;
    msg.transferRights = 0;

    CopyBytes(msg.data, buffer, length);

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

    if (msg.hasTransfer && msg.transferObject) {
      KernelObject* obj = msg.transferObject;
      UInt32 rights = msg.transferRights;
      UInt32 handleValue = 0;

      Task::ControlBlock* tcb = Task::GetCurrent();

      if (tcb && tcb->handleTable) {
        handleValue = tcb->handleTable->Create(obj->type, obj, rights);
      }

      obj->Release();
      msg.transferObject = nullptr;
      msg.hasTransfer = false;
      msg.transferRights = 0;

      UInt32 payload[2] = { 1, handleValue };

      CopyBytes(msg.data, payload, sizeof(payload));
      msg.length = sizeof(payload);
      outLength = msg.length;
    }

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyBytes(outBuffer, msg.data, toCopy);

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

    if (msg.hasTransfer && msg.transferObject) {
      KernelObject* obj = msg.transferObject;
      UInt32 rights = msg.transferRights;
      UInt32 handleValue = 0;

      Task::ControlBlock* tcb = Task::GetCurrent();

      if (tcb && tcb->handleTable) {
        handleValue = tcb->handleTable->Create(obj->type, obj, rights);
      }

      obj->Release();
      msg.transferObject = nullptr;
      msg.hasTransfer = false;
      msg.transferRights = 0;

      UInt32 payload[2] = { 1, handleValue };

      CopyBytes(msg.data, payload, sizeof(payload));
      msg.length = sizeof(payload);
      outLength = msg.length;
    }

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyBytes(outBuffer, msg.data, toCopy);

    port->head = (port->head + 1) % maxQueueDepth;
    --port->count;

    return true;
  }

  bool IPC::DestroyPort(UInt32 portId) {
    Port* port = FindPort(portId);

    if (!port) {
      return false;
    }

    if (port->object) {
      port->object->Release();
    }

    port->used = false;
    port->id = 0;
    port->ownerTaskId = 0;
    port->object = nullptr;
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

  bool IPC::SendHandle(
    UInt32 portId,
    UInt32 senderId,
    KernelObject* object,
    UInt32 rights
  ) {
    if (!object) {
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
    msg.length = sizeof(UInt32) * 2;
    msg.hasTransfer = true;
    msg.transferObject = object;
    msg.transferRights = rights;

    object->AddRef();

    UInt32 payload[2] = { 1, 0 };

    CopyBytes(msg.data, payload, sizeof(payload));

    port->tail = (port->tail + 1) % maxQueueDepth;
    ++port->count;

    return true;
  }

  IPCPortObject* IPC::GetPortObject(UInt32 portId) {
    Port* port = FindPort(portId);

    if (!port) {
      return nullptr;
    }

    return port->object;
  }
}
