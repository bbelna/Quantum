/**
 * @file Libraries/Quantum/Include/ABI/Coordinator.hpp
 * @brief Coordinator IPC payloads.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Types.hpp"

namespace Quantum::ABI::Coordinator {
  /**
   * Coordinator readiness message.
   */
  struct ReadyMessage {
    /**
     * Device identifier.
     */
    UInt32 deviceId;

    /**
     * Ready state (1 = ready).
     */
    UInt32 state;
  };
}
