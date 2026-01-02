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
#include "Sync/ScopedLock.hpp"
#include "Sync/ScopedIRQLock.hpp"
#include "Task.hpp"
#include "WaitQueue.hpp"

namespace Quantum::System::Kernel {
  using ::Quantum::CopyBytes;
  using Objects::IPCPortObject;
  using Objects::KernelObject;

  bool IPC::ConsumeIRQPending(IPC::Port& port, IPC::Message& msg) {
    if (port.irqPayloadLength == 0) {
      return false;
    }

    UInt32 expected = port.irqPending.Load();

    while (expected > 0) {
      UInt32 desired = expected - 1;

      if (port.irqPending.CompareExchange(expected, desired)) {
        msg.senderId = port.irqSenderId;
        msg.length = port.irqPayloadLength;
        msg.hasTransfer = false;
        msg.transferObject = nullptr;
        msg.transferRights = 0;

        CopyBytes(msg.data, port.irqPayload, msg.length);

        return true;
      }
    }

    return false;
  }

  IPC::Port* IPC::FindPort(UInt32 id) {
    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (_ports[i].used && _ports[i].id == id) {
        return &_ports[i];
      }
    }

    return nullptr;
  }

  UInt32 IPC::CreatePort() {
    Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);

    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (!_ports[i].used) {
        _ports[i].used = true;
        _ports[i].id = _nextPortId++;
        _ports[i].ownerTaskId = Task::GetCurrentId();
        _ports[i].object = new IPCPortObject(_ports[i].id);
        _ports[i].head = 0;
        _ports[i].tail = 0;
        _ports[i].count = 0;
        _ports[i].lock.Initialize();
        _ports[i].sendWait.Initialize();
        _ports[i].recvWait.Initialize();
        _ports[i].irqPending.Store(0);
        _ports[i].irqSenderId = 0;
        _ports[i].irqPayloadLength = 0;

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

    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    for (;;) {
      {
        Sync::ScopedLock<Sync::SpinLock> guard(port->lock);

        if (!port->used) {
          return false;
        }

        if (port->count < maxQueueDepth) {
          Message& msg = port->queue[port->tail];

          msg.senderId = senderId;
          msg.length = length;
          msg.hasTransfer = false;
          msg.transferObject = nullptr;
          msg.transferRights = 0;

          CopyBytes(msg.data, buffer, length);

          port->tail = (port->tail + 1) % maxQueueDepth;
          ++port->count;

          port->recvWait.WakeOne();

          return true;
        }
      }

      port->sendWait.WaitTicks(1);
    }

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

    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    Message msg = {};

    for (;;) {
      {
        Sync::ScopedLock<Sync::SpinLock> guard(port->lock);

        if (!port->used) {
          return false;
        }

        if (port->count > 0) {
          msg = port->queue[port->head];

          port->head = (port->head + 1) % maxQueueDepth;
          --port->count;

          port->sendWait.WakeOne();

          break;
        }

        if (ConsumeIRQPending(*port, msg)) {
          break;
        }
      }

      port->recvWait.WaitTicks(1);
    }

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

      UInt32 payload[2] = { 1, handleValue };

      CopyBytes(msg.data, payload, sizeof(payload));

      msg.length = sizeof(payload);
      outLength = msg.length;
    }

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyBytes(outBuffer, msg.data, toCopy);

    return true;
  }

  bool IPC::ReceiveTimeout(
    UInt32 portId,
    UInt32& outSenderId,
    void* outBuffer,
    UInt32 bufferCapacity,
    UInt32& outLength,
    UInt32 timeoutTicks
  ) {
    if (!outBuffer || bufferCapacity == 0) {
      return false;
    }

    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    Message msg = {};
    UInt32 remaining = timeoutTicks;

    for (;;) {
      {
        Sync::ScopedLock<Sync::SpinLock> guard(port->lock);

        if (!port->used) {
          return false;
        }

        if (port->count > 0) {
          msg = port->queue[port->head];

          port->head = (port->head + 1) % maxQueueDepth;
          --port->count;

          port->sendWait.WakeOne();

          break;
        }

        if (ConsumeIRQPending(*port, msg)) {
          break;
        }
      }

      if (remaining == 0) {
        return false;
      }

      bool woken = port->recvWait.WaitTicks(1);

      if (!woken && remaining > 0) {
        remaining -= 1;
      }
    }

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

      UInt32 payload[2] = { 1, handleValue };

      CopyBytes(msg.data, payload, sizeof(payload));

      msg.length = sizeof(payload);
      outLength = msg.length;
    }

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyBytes(outBuffer, msg.data, toCopy);

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

    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    Message msg = {};

    {
      Sync::ScopedLock<Sync::SpinLock> guard(port->lock);

      if (!port->used || port->count == 0) {
        if (ConsumeIRQPending(*port, msg)) {
          outSenderId = msg.senderId;
          outLength = msg.length;

          UInt32 toCopy = msg.length < bufferCapacity
            ? msg.length
            : bufferCapacity;

          CopyBytes(outBuffer, msg.data, toCopy);

          return true;
        }

        return false;
      }

      msg = port->queue[port->head];

      port->head = (port->head + 1) % maxQueueDepth;
      --port->count;

      port->sendWait.WakeOne();
    }

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

      UInt32 payload[2] = { 1, handleValue };

      CopyBytes(msg.data, payload, sizeof(payload));

      msg.length = sizeof(payload);
      outLength = msg.length;
    }

    UInt32 toCopy = msg.length < bufferCapacity ? msg.length : bufferCapacity;

    CopyBytes(outBuffer, msg.data, toCopy);

    return true;
  }

  bool IPC::TrySend(
    UInt32 portId,
    UInt32 senderId,
    const void* buffer,
    UInt32 length
  ) {
    if (!buffer || length == 0 || length > maxPayloadBytes) {
      return false;
    }

    if (!_portsLock.TryAcquire()) {
      return false;
    }

    Port* port = FindPort(portId);

    _portsLock.Release();

    if (!port) {
      return false;
    }

    if (!port->lock.TryAcquire()) {
      if (port->irqPayloadLength != 0) {
        port->irqPending.FetchAdd(1);
        port->recvWait.WakeOne();

        return true;
      }

      return false;
    }

    if (!port->used) {
      port->lock.Release();

      return false;
    }

    if (port->count >= maxQueueDepth) {
      port->lock.Release();

      if (port->irqPayloadLength != 0) {
        port->irqPending.FetchAdd(1);
        port->recvWait.WakeOne();

        return true;
      }

      return false;
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
    port->lock.Release();

    port->recvWait.WakeOne();

    return true;
  }

  bool IPC::DestroyPort(UInt32 portId) {
    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    {
      Sync::ScopedLock<Sync::SpinLock> guard(port->lock);

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
      port->irqPending.Store(0);
      port->irqSenderId = 0;
      port->irqPayloadLength = 0;
    }

    port->sendWait.WakeAll();
    port->recvWait.WakeAll();

    return true;
  }

  bool IPC::ConfigureIRQPayload(
    UInt32 portId,
    const void* buffer,
    UInt32 length,
    UInt32 senderId
  ) {
    if (!buffer || length == 0 || length > maxPayloadBytes) {
      return false;
    }

    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    {
      Sync::ScopedIRQLock<Sync::SpinLock> guard(port->lock);

      if (!port->used) {
        return false;
      }

      port->irqSenderId = senderId;
      port->irqPayloadLength = length;
      CopyBytes(port->irqPayload, buffer, length);
      port->irqPending.Store(0);
    }

    return true;
  }

  bool IPC::GetPortOwner(UInt32 portId, UInt32& outOwnerId) {
    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

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

    Port* port = nullptr;

    {
      Sync::ScopedIRQLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return false;
    }

    for (;;) {
      {
        Sync::ScopedLock<Sync::SpinLock> guard(port->lock);

        if (!port->used) {
          return false;
        }

        if (port->count < maxQueueDepth) {
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

          port->recvWait.WakeOne();

          return true;
        }
      }

      port->sendWait.WaitTicks(1);
    }

    return true;
  }

  IPCPortObject* IPC::GetPortObject(UInt32 portId) {
    Port* port = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_portsLock);
      port = FindPort(portId);
    }

    if (!port) {
      return nullptr;
    }

    return port->object;
  }
}
