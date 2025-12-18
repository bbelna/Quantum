/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/Types/SystemCall.hpp
 * System call identifiers.
 */

#pragma once

namespace Quantum::ABI::Types {
  /**
   * System call identifiers.
   */
  enum SystemCall {
    Write = 0,
    Exit = 1,
    Yield = 2
  };
}
