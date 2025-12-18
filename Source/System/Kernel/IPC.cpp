/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/IPC.cpp
 * Simple kernel IPC primitives (ports + message queues).
 */

#include <IPC.hpp>
#include <Task.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel {
  namespace {
    struct Message {
      UInt32 SenderId;
      UInt32 Length;
      UInt8 Data[IPC::MaxPayloadBytes];
    };

    struct Port {
      bool Used;
      UInt32 Id;
      UInt32 OwnerTaskId;
      UInt32 Head;
      UInt32 Tail;
      UInt32 Count;
      Message Queue[IPC::MaxQueueDepth];
    };

    constexpr UInt32 _maxPorts = 16;
    Port _ports[_maxPorts] = {};
    UInt32 _nextPortId = 1;

    Port* FindPort(UInt32 id) {
      for (UInt32 i = 0; i < _maxPorts; ++i) {
        if (_ports[i].Used && _ports[i].Id == id) {
          return &_ports[i];
        }
      }

      return nullptr;
    }

    void CopyPayload(void* dest, const void* src, UInt32 length) {
      auto* d = reinterpret_cast<UInt8*>(dest);
      auto* s = reinterpret_cast<const UInt8*>(src);

      for (UInt32 i = 0; i < length; ++i) {
        d[i] = s[i];
      }
    }
  }

  UInt32 IPC::CreatePort() {
    for (UInt32 i = 0; i < _maxPorts; ++i) {
      if (!_ports[i].Used) {
        _ports[i].Used = true;
        _ports[i].Id = _nextPortId++;
        _ports[i].OwnerTaskId = Task::GetCurrentId();
        _ports[i].Head = 0;
        _ports[i].Tail = 0;
        _ports[i].Count = 0;

        return _ports[i].Id;
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
    if (!buffer || length == 0 || length > MaxPayloadBytes) {
      return false;
    }

    Port* port = FindPort(portId);

    if (!port) {
      return false;
    }

    // block (cooperatively) if queue full
    while (port->Count >= MaxQueueDepth) {
      Task::Yield();
    }

    Message& msg = port->Queue[port->Tail];

    msg.SenderId = senderId;
    msg.Length = length;
    CopyPayload(msg.Data, buffer, length);

    port->Tail = (port->Tail + 1) % MaxQueueDepth;
    ++port->Count;

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

    while (port->Count == 0) {
      Task::Yield();
    }

    Message& msg = port->Queue[port->Head];

    outSenderId = msg.SenderId;
    outLength = msg.Length;

    UInt32 toCopy = msg.Length < bufferCapacity ? msg.Length : bufferCapacity;
    CopyPayload(outBuffer, msg.Data, toCopy);

    port->Head = (port->Head + 1) % MaxQueueDepth;
    --port->Count;

    return true;
  }
}
