/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Helpers/AlignHelper.hpp
 * Alignment helper utilities.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::System::Kernel::Helpers {
  /**
   * Alignment helper utilities.
   */
  class AlignHelper {
    public:
      /**
       * Aligns a value up to the next multiple of alignment.
       * @param value
       *   Value to align.
       * @param alignment
       *   Alignment boundary (power of two).
       * @return
       *   Aligned value.
       */
      static constexpr UInt32 Up(UInt32 value, UInt32 alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
      }

      /**
       * Aligns a value down to the nearest alignment boundary.
       * @param value
       *   Value to align.
       * @param alignment
       *   Alignment boundary (power of two).
       * @return
       *   Aligned value at or below input.
       */
      static constexpr UInt32 Down(UInt32 value, UInt32 alignment) {
        return value & ~(alignment - 1);
      }
  };
}
