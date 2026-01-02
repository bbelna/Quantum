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
         * Input event delivery.
         */
        Event = 0,

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
       * Broker status codes.
       */
      enum class Status : UInt32 {
        /**
         * Operation successful.
         */
        Ok = 0,

        /**
         * Invalid request.
         */
        Invalid = 1,

        /**
         * Subscription already exists.
         */
        Full = 2,

        /**
         * Subscription not found.
         */
        NotFound = 3
      };

      /**
       * Default timeout in ticks for broker operations.
       */
      static constexpr UInt32 requestTimeoutTicks = 500;

      /**
       * Subscription request payload.
       */
      struct SubscribeMessage {
        /**
         * Operation identifier.
         */
        Operation op;

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
        Operation op;

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
        return Subscribe(portId, requestTimeoutTicks);
      }

      /**
       * Subscribes to the global input stream with a timeout.
       * @param portId
       *   IPC port owned by the caller.
       * @param timeoutTicks
       *   Maximum number of ticks to wait for ack.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Subscribe(UInt32 portId, UInt32 timeoutTicks) {
        if (portId == 0) {
          return 1;
        }

        IPC::Handle inputHandle = IPC::OpenPort(
          static_cast<UInt32>(IPC::Ports::Input),
          static_cast<UInt32>(IPC::Right::Send)
        );

        if (inputHandle == 0) {
          return 1;
        }

        SubscribeMessage request {};
        IPC::Message msg {};

        request.op = Operation::Subscribe;
        request.portId = portId;

        msg.length = sizeof(request);

        for (UInt32 i = 0; i < msg.length; ++i) {
          msg.payload[i] = reinterpret_cast<UInt8*>(&request)[i];
        }

        UInt32 status = IPC::Send(inputHandle, msg);

        IPC::CloseHandle(inputHandle);

        if (status != 0) {
          return status;
        }

        IPC::Message reply {};

        if (IPC::ReceiveTimeout(portId, reply, timeoutTicks) != 0) {
          return 1;
        }

        if (reply.length < sizeof(UInt32)) {
          return 1;
        }

        UInt32 result = 1;

        ::Quantum::CopyBytes(&result, reply.payload, sizeof(result));

        return result;
      }

      /**
       * Unsubscribes from the global input stream.
       * @param portId
       *   IPC port owned by the caller.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Unsubscribe(UInt32 portId) {
        return Unsubscribe(portId, requestTimeoutTicks);
      }

      /**
       * Unsubscribes from the global input stream with a timeout.
       * @param portId
       *   IPC port owned by the caller.
       * @param timeoutTicks
       *   Maximum number of ticks to wait for ack.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Unsubscribe(UInt32 portId, UInt32 timeoutTicks) {
        if (portId == 0) {
          return 1;
        }

        IPC::Handle inputHandle = IPC::OpenPort(
          static_cast<UInt32>(IPC::Ports::Input),
          static_cast<UInt32>(IPC::Right::Send)
        );

        if (inputHandle == 0) {
          return 1;
        }

        SubscribeMessage request {};
        IPC::Message msg {};

        request.op = Operation::Unsubscribe;
        request.portId = portId;

        msg.length = sizeof(request);

        for (UInt32 i = 0; i < msg.length; ++i) {
          msg.payload[i] = reinterpret_cast<UInt8*>(&request)[i];
        }

        UInt32 status = IPC::Send(inputHandle, msg);

        IPC::CloseHandle(inputHandle);

        if (status != 0) {
          return status;
        }

        IPC::Message reply {};

        if (IPC::ReceiveTimeout(portId, reply, timeoutTicks) != 0) {
          return 1;
        }

        if (reply.length < sizeof(UInt32)) {
          return 1;
        }

        UInt32 result = 1;

        ::Quantum::CopyBytes(&result, reply.payload, sizeof(result));

        return result;
      }
  };
}

