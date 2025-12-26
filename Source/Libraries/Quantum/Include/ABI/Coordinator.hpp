/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/Coordinator.hpp
 * Coordinator IPC payloads.
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
