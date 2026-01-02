/**
 * @file System/Coordinator/Include/Input.hpp
 * @brief Coordinator input broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/Input.hpp>
#include <Types.hpp>

namespace Quantum::System::Coordinator {
  /**
   * Coordinator input broker.
   */
  class Input {
    public:
      /**
       * Initializes the input broker port.
       */
      static void Initialize();

      /**
       * Processes pending input subscriptions and events.
       */
      static void ProcessPending();

    private:
      /**
       * Input broker port id.
       */
      inline static UInt32 _portId = 0;

      /**
       * Input broker port handle.
       */
      inline static UInt32 _portHandle = 0;

      /**
       * Maximum number of input subscribers.
       */
      static constexpr UInt32 _maxSubscribers = 16;

      /**
       * Subscriber port ids.
       */
      inline static UInt32 _subscriberPorts[_maxSubscribers] = {};

      /**
       * Registers a subscriber port.
       * @param portId
       *   Subscriber port identifier.
       */
      static ABI::Input::Status AddSubscriber(UInt32 portId);

      /**
       * Removes a subscriber port.
       * @param portId
       *   Subscriber port identifier.
       */
      static ABI::Input::Status RemoveSubscriber(UInt32 portId);
  };
}
