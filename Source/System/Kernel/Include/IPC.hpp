/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/IPC.hpp
 * Simple kernel IPC primitives (ports + message queues).
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::IPC {
  /**
   * Maximum payload size in bytes for an IPC message.
   */
  constexpr UInt32 MaxPayloadBytes = 256;

  /**
   * Maximum number of queued messages per port.
   */
  constexpr UInt32 MaxQueueDepth = 16;

  UInt32 CreatePort();
  bool Send(UInt32 portId, UInt32 senderId, const void* buffer, UInt32 length);
  bool Receive(
    UInt32 portId,
    UInt32& outSenderId,
    void* outBuffer,
    UInt32 bufferCapacity,
    UInt32& outLength
  );
}
