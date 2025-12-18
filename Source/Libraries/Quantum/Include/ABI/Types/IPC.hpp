/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/Types/IPC.hpp
 * IPC message definitions shared between kernel and userland.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::ABI::Types::IPC {
  /**
   * Maximum payload size for an IPC message.
   */
  constexpr UInt32 MaxPayloadBytes = 256;

  /**
   * IPC message layout.
   */
  struct Message {
    /**
     * Sender task identifier (set by the kernel).
     */
    UInt32 SenderId;

    /**
     * Length of the payload in bytes.
     */
    UInt32 Length;

    /**
     * Message payload bytes.
     */
    UInt8 Payload[MaxPayloadBytes];
  };
}
