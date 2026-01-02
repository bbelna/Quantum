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
        /**
         * Opens a block device handle.
         */
        OpenBlock = 1,

        /**
         * Opens an input device handle.
         */
        OpenInput = 2
      };

      /**
       * Default timeout in ticks for broker requests.
       */
      static constexpr UInt32 requestTimeoutTicks = 500;

      /**
       * Device broker request message.
       */
      struct Request {
        /**
         * Operation type.
         */
        Operation op;

        /**
         * Device identifier.
         */
        UInt32 deviceId;

        /**
         * Requested rights mask.
         */
        UInt32 rights;

        /**
         * Reply port identifier.
         */
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
      /**
       * Opens a device handle via the coordinator.
       * @param op
       *   Operation type.
       * @param deviceId
       *   Device identifier.
       * @param rights
       *   Rights mask.
       * @return
       *   Handle on success; 0 on failure.
       */
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
          static_cast<UInt32>(IPC::Right::Receive)
            | static_cast<UInt32>(IPC::Right::Manage)
            | static_cast<UInt32>(IPC::Right::Send)
        );

        if (replyHandle == 0) {
          IPC::DestroyPort(replyPortId);

          return 0;
        }

        IPC::Handle brokerHandle = IPC::OpenPort(
          static_cast<UInt32>(IPC::Ports::Devices),
          static_cast<UInt32>(IPC::Right::Send)
        );

        if (brokerHandle == 0) {
          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        Request request {};

        request.op = op;
        request.deviceId = deviceId;
        request.rights = rights;
        request.replyPortId = 0;

        IPC::Message msg {};

        msg.length = sizeof(request);

        ::Quantum::CopyBytes(msg.payload, &request, sizeof(request));

        if (IPC::SendHandle(
          brokerHandle,
          replyHandle,
          static_cast<UInt32>(IPC::Right::Send)
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
        UInt32 remaining = requestTimeoutTicks;

        while (remaining > 0) {
          if (IPC::ReceiveTimeout(replyHandle, reply, 1) == 0) {
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

          remaining -= 1;
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

