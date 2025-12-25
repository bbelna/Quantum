/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/IPC.hpp
 * IPC syscall wrappers.
 */

#pragma once

#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * IPC syscall wrappers.
   */
  class IPC {
    public:
      /**
       * Maximum payload size for an IPC message.
       */
      static constexpr UInt32 maxPayloadBytes = 1024;

      /**
       * Coordinator port identifiers.
       */
      struct Ports {
        /**
         * IRQ routing control port.
         */
        static constexpr UInt32 IRQ = 1;

        /**
         * File system broker port.
         */
        static constexpr UInt32 FileSystem = 2;
      };

      /**
       * IPC message layout.
       */
      struct Message {
        /**
         * Sender task identifier (set by the kernel).
         */
        UInt32 senderId;

        /**
         * Length of the payload in bytes.
         */
        UInt32 length;

        /**
         * Message payload bytes.
         */
        UInt8 payload[maxPayloadBytes];
      };

      /**
       * Creates a new IPC port owned by the caller.
       * @return
       *   Port id on success, 0 on failure.
       */
      static UInt32 CreatePort() {
        return InvokeSystemCall(SystemCall::IPC_CreatePort);
      }

      /**
       * Sends a message to a port.
       * @param portId
       *   Target port.
       * @param message
       *   Message to send; Length must be <= MaxPayloadBytes.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Send(
        UInt32 portId,
        const Message& message
      ) {
        return InvokeSystemCall(
          SystemCall::IPC_Send,
          portId,
          reinterpret_cast<UInt32>(&message),
          0
        );
      }

      /**
       * Receives a message from a port (blocking).
       * @param portId
       *   Port to receive from.
       * @param outMessage
       *   Receives the message contents.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Receive(
        UInt32 portId,
        Message& outMessage
      ) {
        return InvokeSystemCall(
          SystemCall::IPC_Receive,
          portId,
          reinterpret_cast<UInt32>(&outMessage),
          0
        );
      }

      /**
       * Attempts to receive a message without blocking.
       * @param portId
       *   Port to receive from.
       * @param outMessage
       *   Receives the message contents.
       * @return
       *   0 on success, non-zero when no message or on failure.
       */
      static UInt32 TryReceive(
        UInt32 portId,
        Message& outMessage
      ) {
        return InvokeSystemCall(
          SystemCall::IPC_TryReceive,
          portId,
          reinterpret_cast<UInt32>(&outMessage),
          0
        );
      }
  };
}
