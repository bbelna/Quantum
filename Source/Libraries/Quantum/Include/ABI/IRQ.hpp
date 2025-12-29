/**
 * @file Libraries/Quantum/Include/ABI/IRQ.hpp
 * @brief IRQ IPC helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/IPC.hpp"
#include "ABI/Task.hpp"
#include "Bytes.hpp"
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
         * Requests IRQ routing to a port.
         */
        Register = 1
      };

      /**
       * IRQ message payload.
       */
      struct Message {
        /**
         * Operation identifier (0 for IRQ notification).
         */
        UInt32 op;

        /**
         * IRQ line number.
         */
        UInt32 irq;

        /**
         * Target port id.
         */
        UInt32 portId;

        /**
         * Reply port id for acknowledgements.
         */
        UInt32 replyPortId;

        /**
         * Reserved payload data.
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

        if (replyPortId == 0) {
          return 1;
        }

        Message request {};
        IPC::Message msg {};

        request.op = static_cast<UInt32>(Operation::Register);
        request.irq = irq;
        request.portId = portId;
        request.replyPortId = replyPortId;
        request.data = 0;

        msg.length = sizeof(request);

        ::Quantum::CopyBytes(msg.payload, &request, msg.length);

        IPC::Send(IPC::Ports::IRQ, msg);

        IPC::Message reply {};

        for (UInt32 i = 0; i < 1024; ++i) {
          if (IPC::TryReceive(replyPortId, reply) == 0) {
            if (reply.length >= sizeof(UInt32)) {
              UInt32 status = 0;

              ::Quantum::CopyBytes(&status, reply.payload, sizeof(status));

              IPC::DestroyPort(replyPortId);

              return status;
            }

            IPC::DestroyPort(replyPortId);

            return 1;
          }

          if ((i & 0x3F) == 0) {
            Task::Yield();
          }
        }

        IPC::DestroyPort(replyPortId);

        return 1;
      }
  };
}
