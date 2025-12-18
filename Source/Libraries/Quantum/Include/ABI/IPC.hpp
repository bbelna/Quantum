/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/IPC.hpp
 * IPC syscall wrappers.
 */

#pragma once

#include <ABI/InvokeSystemCall.hpp>
#include <ABI/Types/IPC.hpp>
#include <ABI/Types/SystemCall.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::ABI {
  /**
   * IPC syscall wrappers.
   */
  class IPC {
    public:
      /**
       * Creates a new IPC port owned by the caller.
       * @return
       *   Port id on success, 0 on failure.
       */
      static UInt32 CreatePort() {
        return InvokeSystemCall(Types::SystemCall::IPC_CreatePort);
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
        const Types::IPC::Message& message
      ) {
        return InvokeSystemCall(
          Types::SystemCall::IPC_Send,
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
        Types::IPC::Message& outMessage
      ) {
        return InvokeSystemCall(
          Types::SystemCall::IPC_Receive,
          portId,
          reinterpret_cast<UInt32>(&outMessage),
          0
        );
      }
  };
}
