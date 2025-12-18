/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/Types/ABI/SystemCallId.hpp
 * System call identifiers.
 */

#pragma once

namespace Quantum::Types::ABI {
  /**
   * System call identifiers.
   */
  enum SystemCallId {
    Write = 0,
    Exit = 1,
    Yield = 2
  };
}
