/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/IRQ.hpp
 * IRQ IPC helpers.
 */

#pragma once

#include "ABI/IPC.hpp"
#include "ABI/Task.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * IRQ IPC helpers.
   */
  class IRQ {
    public:
      /**
       * IRQ routing operation identifiers.
       */
      enum class Operation : UInt32 {
        /**
         * requests IRQ routing to a port.
         */
        Register = 1
      };

      /**
       * IRQ message payload.
       */
      struct Message {
        /**
         * operation identifier (0 for IRQ notification).
         */
        UInt32 op;

        /**
         * IRQ line number.
         */
        UInt32 irq;

        /**
         * target port id.
         */
        UInt32 portId;

        /**
         * reply port id for acknowledgements.
         */
        UInt32 replyPortId;

        /**
         * reserved payload data.
         */
        UInt32 data;
      };

      /**
       * Registers an IRQ routing port with the coordinator.
       * @param irq
       *   IRQ line number.
       * @param portId
       *   IPC port owned by the caller.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Register(UInt32 irq, UInt32 portId) {
        if (portId == 0) {
          return 1;
        }

        UInt32 replyPortId = IPC::CreatePort();
        Message request {};
        IPC::Message msg {};

        request.op = static_cast<UInt32>(Operation::Register);
        request.irq = irq;
        request.portId = portId;
        request.replyPortId = replyPortId;
        request.data = 0;

        msg.length = sizeof(request);

        CopyBytes(msg.payload, &request, msg.length);

        IPC::Send(IPC::Ports::IRQ, msg);

        if (replyPortId == 0) {
          return 1;
        }

        IPC::Message reply {};

        for (UInt32 i = 0; i < 1024; ++i) {
          if (IPC::TryReceive(replyPortId, reply) == 0) {
            if (reply.length >= sizeof(UInt32)) {
              UInt32 status = 0;

              CopyBytes(&status, reply.payload, sizeof(status));

              return status;
            }

            return 1;
          }

          if ((i & 0x3F) == 0) {
            Task::Yield();
          }
        }

        return 1;
      }

    private:
      /**
       * Copies bytes between buffers.
       */
      static void CopyBytes(void* dest, const void* src, UInt32 length) {
        auto* d = reinterpret_cast<UInt8*>(dest);
        auto* s = reinterpret_cast<const UInt8*>(src);

        for (UInt32 i = 0; i < length; ++i) {
          d[i] = s[i];
        }
      }
  };
}
