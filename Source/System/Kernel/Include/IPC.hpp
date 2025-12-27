/**
 * @file System/Kernel/Include/IPC.hpp
 * @brief Inter-process communication (IPC) handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * IPC subsystem.
   */
  class IPC {
    public:
      /**
       * Maximum payload size in bytes for an IPC message.
       */
      static constexpr UInt32 maxPayloadBytes = 1024;

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

      /**
       * Attempts to receive a message without blocking.
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
       *   True on success; false if no message or invalid arguments/port.
       */
      static bool TryReceive(
        UInt32 portId,
        UInt32& outSenderId,
        void* outBuffer,
        UInt32 bufferCapacity,
        UInt32& outLength
      );

      /**
       * Destroys an IPC port and frees its slot.
       * @param portId
       *   Port to destroy.
       * @return
       *   True on success; false if not found.
       */
      static bool DestroyPort(UInt32 portId);

      /**
       * Retrieves the owner task id for a port.
       * @param portId
       *   Port identifier to query.
       * @param outOwnerId
       *   Receives the owner task id.
       * @return
       *   True if the port exists; false otherwise.
       */
      static bool GetPortOwner(UInt32 portId, UInt32& outOwnerId);

    private:
      /**
       * IPC message descriptor stored in each port queue.
       */
      struct Message {
        /**
         * Identifier of the sending task.
         */
        UInt32 senderId;

        /**
         * Length of the payload in bytes.
         */
        UInt32 length;

        /**
         * Message payload data.
         */
        UInt8 data[maxPayloadBytes];
      };

      /**
       * IPC port state tracked by the kernel.
       */
      struct Port {
        /**
         * Whether this port slot is in use.
         */
        bool used;

        /**
         * Unique port identifier.
         */
        UInt32 id;

        /**
         * Identifier of the owning task.
         */
        UInt32 ownerTaskId;

        /**
         * Head index in the message queue.
         */
        UInt32 head;

        /**
         * Tail index in the message queue.
         */
        UInt32 tail;

        /**
         * Number of messages currently queued.
         */
        UInt32 count;

        /**
         * Message queue.
         */
        Message queue[maxQueueDepth];
      };

      /**
       * Maximum number of ports supported by the kernel.
       */
      static constexpr UInt32 _maxPorts = 16;

      /**
       * Global IPC port table.
       */
      inline static Port _ports[_maxPorts] = {};

      /**
       * Next port id to hand out.
       */
      inline static UInt32 _nextPortId = 1;

      /**
       * Finds a port by id.
       */
      static Port* FindPort(UInt32 id);

      /**
       * Copies a message payload into a destination buffer.
       */
      static void CopyPayload(void* dest, const void* src, UInt32 length);
  };
}
