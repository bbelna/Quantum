/**
 * @file Libraries/Quantum/Include/ABI/IRQ.hpp
 * @brief IRQ IPC helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/Handle.hpp"
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
       * IRQ handle type.
       */
      using Handle = UInt32;
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
         * Reply port id for acknowledgements (0 when using handle transfer).
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
        return Register(irq, portId, nullptr);
      }

      /**
       * Registers an IRQ routing port with the coordinator.
       * @param irq
       *   IRQ line number.
       * @param portId
       *   IPC port owned by the caller.
       * @param outHandle
       *   Receives an IRQ handle (optional).
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Register(UInt32 irq, UInt32 portId, Handle* outHandle) {
        if (portId == 0) {
          return 1;
        }

        UInt32 replyPortId = IPC::CreatePort();

        if (replyPortId == 0) {
          return 1;
        }

        IPC::Handle irqHandle = IPC::OpenPort(
          IPC::Ports::IRQ,
          IPC::RightSend
        );

        if (irqHandle == 0) {
          IPC::DestroyPort(replyPortId);

          return 1;
        }

        IPC::Handle replyHandle = IPC::OpenPort(
          replyPortId,
          IPC::RightReceive | IPC::RightManage | IPC::RightSend
        );

        if (replyHandle == 0) {
          IPC::CloseHandle(irqHandle);
          IPC::DestroyPort(replyPortId);

          return 1;
        }

        Message request {};
        IPC::Message msg {};

        request.op = static_cast<UInt32>(Operation::Register);
        request.irq = irq;
        request.portId = portId;
        request.replyPortId = 0;
        request.data = 0;

        msg.length = sizeof(request);

        ::Quantum::CopyBytes(msg.payload, &request, msg.length);

        if (IPC::SendHandle(
          irqHandle,
          replyHandle,
          IPC::RightSend
        ) != 0) {
          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);
          IPC::CloseHandle(irqHandle);

          return 1;
        }

        IPC::Send(irqHandle, msg);

        IPC::Message reply {};
        Handle receivedHandle = 0;

        for (UInt32 i = 0; i < 1024; ++i) {
          if (IPC::TryReceive(replyHandle, reply) == 0) {
            Handle transferHandle = 0;

            if (IPC::TryGetHandleMessage(reply, transferHandle)) {
              if (receivedHandle != 0) {
                ABI::Handle::Close(receivedHandle);
              }

              receivedHandle = transferHandle;

              continue;
            }

            if (reply.length >= sizeof(UInt32)) {
              UInt32 status = 0;

              ::Quantum::CopyBytes(&status, reply.payload, sizeof(status));

              if (outHandle) {
                *outHandle = receivedHandle;
              } else if (receivedHandle != 0) {
                ABI::Handle::Close(receivedHandle);
              }

              IPC::DestroyPort(replyHandle);
              IPC::CloseHandle(replyHandle);
              IPC::CloseHandle(irqHandle);

              return status;
            }

            if (receivedHandle != 0) {
              ABI::Handle::Close(receivedHandle);
            }

            IPC::DestroyPort(replyHandle);
            IPC::CloseHandle(replyHandle);
            IPC::CloseHandle(irqHandle);

            return 1;
          }

          if ((i & 0x3F) == 0) {
            Task::Yield();
          }
        }

        if (receivedHandle != 0) {
          ABI::Handle::Close(receivedHandle);
        }

        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);
        IPC::CloseHandle(irqHandle);

        return 1;
      }

      /**
       * IRQ register right.
       */
      static constexpr UInt32 RightRegister = 1u << 0;

      /**
       * IRQ unregister right.
       */
      static constexpr UInt32 RightUnregister = 1u << 1;

      /**
       * IRQ enable right.
       */
      static constexpr UInt32 RightEnable = 1u << 2;

      /**
       * IRQ disable right.
       */
      static constexpr UInt32 RightDisable = 1u << 3;

      /**
       * Opens a handle to an IRQ line.
       * @param irq
       *   IRQ line number.
       * @param rights
       *   Rights mask.
       * @return
       *   Handle on success; 0 on failure.
       */
      static Handle Open(UInt32 irq, UInt32 rights) {
        return InvokeSystemCall(SystemCall::IRQ_Open, irq, rights, 0);
      }

      /**
       * Unregisters an IRQ routing port.
       * @param irq
       *   IRQ line number or handle.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Unregister(UInt32 irq) {
        return InvokeSystemCall(SystemCall::IRQ_Unregister, irq, 0, 0);
      }

      /**
       * Enables an IRQ line.
       * @param irq
       *   IRQ line number or handle.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Enable(UInt32 irq) {
        return InvokeSystemCall(SystemCall::IRQ_Enable, irq, 0, 0);
      }

      /**
       * Disables an IRQ line.
       * @param irq
       *   IRQ line number or handle.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Disable(UInt32 irq) {
        return InvokeSystemCall(SystemCall::IRQ_Disable, irq, 0, 0);
      }
  };
}
