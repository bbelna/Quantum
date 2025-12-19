/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/IPC.hpp
 * Simple kernel IPC primitives (ports + message queues).
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * IPC subsystem (simple ports + message queues).
   */
  class IPC {
    public:
      /**
       * Maximum payload size in bytes for an IPC message.
       */
      static constexpr UInt32 maxPayloadBytes = 256;

      /**
       * Maximum number of queued messages per port.
       */
      static constexpr UInt32 maxQueueDepth = 16;

      /**
       * Creates a new IPC port owned by the current task.
       * @return
       *   Port identifier (non-zero) on success; 0 on failure.
       */
      static UInt32 CreatePort();

      /**
       * Sends a message to the given port (blocking if the queue is full).
       * @param portId
       *   Target port.
       * @param senderId
       *   Identifier of the sending task.
       * @param buffer
       *   Pointer to payload data.
       * @param length
       *   Payload length in bytes (<= MaxPayloadBytes).
       * @return
       *   True on success; false on invalid arguments/port.
       */
      static bool Send(
        UInt32 portId,
        UInt32 senderId,
        const void* buffer,
        UInt32 length
      );

      /**
       * Receives a message from the given port (blocking if empty).
       * @param portId
       *   Port to receive from.
       * @param outSenderId
       *   Receives the sender task id.
       * @param outBuffer
       *   Buffer to copy payload into.
       * @param bufferCapacity
       *   Capacity of outBuffer in bytes.
       * @param outLength
       *   Receives the payload length.
       * @return
       *   True on success; false on invalid arguments/port.
       */
      static bool Receive(
        UInt32 portId,
        UInt32& outSenderId,
        void* outBuffer,
        UInt32 bufferCapacity,
        UInt32& outLength
      );
  };
}
