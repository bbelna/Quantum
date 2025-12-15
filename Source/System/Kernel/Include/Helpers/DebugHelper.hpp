//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Helpers/DebugHelper.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Debug/diagnostic helper utilities.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel::Helpers {
  /**
   * Debug/diagnostic helper utilities.
   */
  class DebugHelper {
    public:
      /**
       * Trims a fully qualified source file path to a shorter form by removing
       * any prefix up to and including "/Source/" or "\\Source\\".
       * @param filePath Full path string (may be null).
       * @return Pointer to the trimmed path inside the provided string, or the
       *         original pointer if no trim marker is found.
       */
      static CString TrimSourceFile(CString filePath);
  };
}
