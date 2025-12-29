/**
 * @file Libraries/Quantum/Include/ABI/Input.hpp
 * @brief Input broker IPC helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/Devices/InputDevices.hpp"
#include "ABI/IPC.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * Input broker IPC helpers.
   */
  class Input {
    public:
      /**
       * Input broker operation identifiers.
       */
      enum class Operation : UInt32 {
        /**
         * Subscribe to input events.
         */
        Subscribe = 1,

        /**
         * Unsubscribe from input events.
         */
        Unsubscribe = 2
      };

      /**
       * Subscription request payload.
       */
      struct SubscribeMessage {
        /**
         * Operation identifier.
         */
        UInt32 op;

        /**
         * Subscriber port identifier.
         */
        UInt32 portId;
      };

      /**
       * Input event payload.
       */
      struct EventMessage {
        /**
         * Operation identifier (0 for event delivery).
         */
        UInt32 op;

        /**
         * Input event payload.
         */
        Devices::InputDevices::Event event;
      };

      /**
       * Subscribes to the global input stream.
       * @param portId
       *   IPC port owned by the caller.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Subscribe(UInt32 portId) {
        if (portId == 0) {
          return 1;
        }

        IPC::Handle inputHandle = IPC::OpenPort(
          IPC::Ports::Input,
          IPC::RightSend
        );

        if (inputHandle == 0) {
          return 1;
        }

        SubscribeMessage request {};
        IPC::Message msg {};

        request.op = static_cast<UInt32>(Operation::Subscribe);
        request.portId = portId;

        msg.length = sizeof(request);

        for (UInt32 i = 0; i < msg.length; ++i) {
          msg.payload[i] = reinterpret_cast<UInt8*>(&request)[i];
        }

        UInt32 status = IPC::Send(inputHandle, msg);

        IPC::CloseHandle(inputHandle);

        return status;
      }

      /**
       * Unsubscribes from the global input stream.
       * @param portId
       *   IPC port owned by the caller.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Unsubscribe(UInt32 portId) {
        if (portId == 0) {
          return 1;
        }

        IPC::Handle inputHandle = IPC::OpenPort(
          IPC::Ports::Input,
          IPC::RightSend
        );

        if (inputHandle == 0) {
          return 1;
        }

        SubscribeMessage request {};
        IPC::Message msg {};

        request.op = static_cast<UInt32>(Operation::Unsubscribe);
        request.portId = portId;

        msg.length = sizeof(request);

        for (UInt32 i = 0; i < msg.length; ++i) {
          msg.payload[i] = reinterpret_cast<UInt8*>(&request)[i];
        }

        UInt32 status = IPC::Send(inputHandle, msg);

        IPC::CloseHandle(inputHandle);

        return status;
      }
  };
}
