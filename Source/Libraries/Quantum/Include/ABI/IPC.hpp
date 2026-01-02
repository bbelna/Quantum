/**
 * @file Libraries/Quantum/Include/ABI/IPC.hpp
 * @brief IPC syscall wrappers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/SystemCall.hpp"
#include "Bytes.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * IPC syscall wrappers.
   */
  class IPC {
    public:
      /**
       * IPC handle type.
       */
      using Handle = UInt32;

      /**
       * Maximum payload size for an IPC message.
       */
      static constexpr UInt32 maxPayloadBytes = 1024;

      /**
       * Coordinator port identifiers.
       */
      enum class Ports : UInt32 {
        /**
         * IRQ routing control port.
         */
        IRQ = 1,

        /**
         * File system broker port.
         */
        FileSystem = 2,

        /**
         * Coordinator readiness port.
         */
        CoordinatorReady = 3,

        /**
         * Input broker port.
         */
        Input = 4,

        /**
         * Device broker port.
         */
        Devices = 5
      };

      /**
       * IPC right flags.
       */
      enum class Right : UInt32 {
        /**
         * Send right.
         */
        Send = 1u << 0,

        /**
         * Receive right.
         */
        Receive = 1u << 1,

        /**
         * Manage right.
         */
        Manage = 1u << 2
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
       * IPC handle transfer payload.
       */
      struct HandleMessage {
        /**
         * Handle message operations.
         */
        enum class Operation : UInt32 {
          Transfer = 1
        };

        /**
         * Operation type.
         */
        Operation op;

        /**
         * Transferred handle.
         */
        UInt32 handle;
      };

      /**
       * Attempts to extract a handle transfer message.
       * @param message
       *   IPC message to inspect.
       * @param outHandle
       *   Receives the transferred handle on success.
       * @return
       *   True if the message is a handle transfer; false otherwise.
       */
      static bool TryGetHandleMessage(
        const Message& message,
        Handle& outHandle
      ) {
        if (message.length != sizeof(HandleMessage)) {
          return false;
        }

        HandleMessage transfer {};

        ::Quantum::CopyBytes(
          &transfer,
          message.payload,
          sizeof(HandleMessage)
        );

        if (transfer.op != HandleMessage::Operation::Transfer) {
          return false;
        }

        outHandle = transfer.handle;

        return true;
      }

      /**
       * Creates a new IPC port owned by the caller.
       * @return
       *   Port id on success, 0 on failure.
       */
      static UInt32 CreatePort() {
        return InvokeSystemCall(SystemCall::IPC_CreatePort);
      }

      /**
       * Opens a handle to an existing port id.
       * @param portId
       *   Existing port identifier.
       * @param rights
       *   Rights mask (send/receive/manage).
       * @return
       *   Handle on success; 0 on failure.
       */
      static Handle OpenPort(UInt32 portId, UInt32 rights) {
        return InvokeSystemCall(SystemCall::IPC_OpenPort, portId, rights, 0);
      }

      /**
       * Closes a previously opened IPC handle.
       * @param handle
       *   Handle to close.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 CloseHandle(Handle handle) {
        return InvokeSystemCall(SystemCall::IPC_CloseHandle, handle, 0, 0);
      }

      /**
       * Destroys an IPC port owned by the caller.
       * @param portId
       *   Port id or handle to destroy.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 DestroyPort(UInt32 portId) {
        return InvokeSystemCall(SystemCall::IPC_DestroyPort, portId);
      }

      /**
       * Sends a message to a port.
       * @param portId
       *   Target port id or handle.
       * @param message
       *   Message to send; length must be <= `maxPayloadBytes`.
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
       * Sends a handle to a port (handle is duplicated to the receiver).
       * @param portId
       *   Target port id or handle.
       * @param handle
       *   Handle to transfer.
       * @param rights
       *   Rights mask (0 to keep original rights).
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 SendHandle(
        UInt32 portId,
        Handle handle,
        UInt32 rights = 0
      ) {
        return InvokeSystemCall(
          SystemCall::IPC_SendHandle,
          portId,
          handle,
          rights
        );
      }

      /**
       * Receives a message from a port (blocking).
       * @param portId
       *   Port id or handle to receive from.
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
       * Receives a message with a timeout.
       * @param portId
       *   Port id or handle to receive from.
       * @param outMessage
       *   Receives the message contents.
       * @param timeoutTicks
       *   Maximum number of ticks to wait.
       * @return
       *   0 on success, non-zero on timeout or failure.
       */
      static UInt32 ReceiveTimeout(
        UInt32 portId,
        Message& outMessage,
        UInt32 timeoutTicks
      ) {
        return InvokeSystemCall(
          SystemCall::IPC_ReceiveTimeout,
          portId,
          reinterpret_cast<UInt32>(&outMessage),
          timeoutTicks
        );
      }

      /**
       * Attempts to receive a message without blocking.
       * @param portId
       *   Port id or handle to receive from.
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
