/**
 * @file Libraries/Quantum/Include/ABI/Devices/DeviceBroker.hpp
 * @brief Device handle broker helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/Devices/BlockDevices.hpp"
#include "ABI/Devices/InputDevices.hpp"
#include "ABI/Handle.hpp"
#include "ABI/IPC.hpp"
#include "ABI/Task.hpp"
#include "Bytes.hpp"
#include "Types.hpp"

namespace Quantum::ABI::Devices {
  /**
   * Device handle broker helpers.
   */
  class DeviceBroker {
    public:
      /**
       * Device broker operation identifiers.
       */
      enum class Operation : UInt32 {
        OpenBlock = 1,
        OpenInput = 2
      };

      /**
       * Device broker request message.
       */
      struct Request {
        UInt32 op;
        UInt32 deviceId;
        UInt32 rights;
        UInt32 replyPortId;
      };

      /**
       * Opens a block device handle via the coordinator.
       * @param deviceId
       *   Block device identifier.
       * @param rights
       *   Rights mask.
       * @return
       *   Handle on success; 0 on failure.
       */
      static BlockDevices::Handle OpenBlockDevice(
        UInt32 deviceId,
        UInt32 rights
      ) {
        return OpenDevice(
          Operation::OpenBlock,
          deviceId,
          rights
        );
      }

      /**
       * Opens an input device handle via the coordinator.
       * @param deviceId
       *   Input device identifier.
       * @param rights
       *   Rights mask.
       * @return
       *   Handle on success; 0 on failure.
       */
      static InputDevices::Handle OpenInputDevice(
        UInt32 deviceId,
        UInt32 rights
      ) {
        return OpenDevice(
          Operation::OpenInput,
          deviceId,
          rights
        );
      }

    private:
      static UInt32 OpenDevice(
        Operation op,
        UInt32 deviceId,
        UInt32 rights
      ) {
        IPC::Handle replyPortId = IPC::CreatePort();

        if (replyPortId == 0) {
          return 0;
        }

        IPC::Handle replyHandle = IPC::OpenPort(
          replyPortId,
          IPC::RightReceive | IPC::RightManage | IPC::RightSend
        );

        if (replyHandle == 0) {
          IPC::DestroyPort(replyPortId);

          return 0;
        }

        IPC::Handle brokerHandle = IPC::OpenPort(
          IPC::Ports::Devices,
          IPC::RightSend
        );

        if (brokerHandle == 0) {
          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        Request request {};

        request.op = static_cast<UInt32>(op);
        request.deviceId = deviceId;
        request.rights = rights;
        request.replyPortId = 0;

        IPC::Message msg {};

        msg.length = sizeof(request);
        ::Quantum::CopyBytes(msg.payload, &request, sizeof(request));

        if (IPC::SendHandle(
          brokerHandle,
          replyHandle,
          IPC::RightSend
        ) != 0) {
          IPC::CloseHandle(brokerHandle);
          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        if (IPC::Send(brokerHandle, msg) != 0) {
          IPC::CloseHandle(brokerHandle);
          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        IPC::CloseHandle(brokerHandle);

        IPC::Message reply {};
        UInt32 status = 1;
        UInt32 receivedHandle = 0;

        for (UInt32 i = 0; i < 1024; ++i) {
          if (IPC::TryReceive(replyHandle, reply) == 0) {
            UInt32 transferHandle = 0;

            if (IPC::TryGetHandleMessage(reply, transferHandle)) {
              if (receivedHandle != 0) {
                ABI::Handle::Close(receivedHandle);
              }

              receivedHandle = transferHandle;

              continue;
            }

            if (reply.length >= sizeof(UInt32)) {
              ::Quantum::CopyBytes(&status, reply.payload, sizeof(status));

              break;
            }

            break;
          }

          if ((i & 0x3F) == 0) {
            Task::Yield();
          }
        }

        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);

        if (status != 0) {
          if (receivedHandle != 0) {
            ABI::Handle::Close(receivedHandle);
          }

          return 0;
        }

        return receivedHandle;
      }
  };
}
