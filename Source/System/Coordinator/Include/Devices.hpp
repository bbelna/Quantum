/**
 * @file System/Coordinator/Include/Devices.hpp
 * @brief Coordinator device handle broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Coordinator {
  /**
   * Coordinator device handle broker.
   */
  class Devices {
    public:
      /**
       * Initializes the device broker port.
       */
      static void Initialize();

      /**
       * Processes any pending device broker requests.
       */
      static void ProcessPending();

    private:
      /**
       * Device broker port id.
       */
      inline static UInt32 _portId = 0;

      /**
       * Device broker port handle.
       */
      inline static UInt32 _portHandle = 0;

      /**
       * Pending reply handles awaiting a request.
       */
      struct PendingReply {
        /**
         * Indicates whether this entry is in use.
         */
        bool inUse;

        /**
         * Sender task identifier.
         */
        UInt32 senderId;

        /**
         * Reply handle.
         */
        UInt32 handle;
      };

      /**
       * Maximum number of pending reply handles.
       */
      static constexpr UInt32 _maxPendingReplies = 16;

      /**
       * Pending reply handle storage.
       */
      inline static PendingReply _pendingReplies[_maxPendingReplies] = {};

      /**
       * Stores a pending reply handle for a sender.
       * @param senderId
       *   Sender task identifier.
       * @param handle
       *   Reply handle.
       */
      static void StorePendingReply(UInt32 senderId, UInt32 handle);

      /**
       * Takes a pending reply handle for a sender.
       * @param senderId
       *   Sender task identifier.
       * @return
       *   Reply handle, or 0 if none pending.
       */
      static UInt32 TakePendingReply(UInt32 senderId);
  };
}
