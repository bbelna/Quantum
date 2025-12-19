/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Helpers/DebugHelper.hpp
 * Debug/diagnostic helper utilities.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Helpers {
  /**
   * Debug/diagnostic helper utilities.
   */
  class DebugHelper {
    public:
      /**
       * Trims a fully qualified source file path to a shorter form by removing
       * any prefix up to and including "Source".
       * @param filePath
       *   Full path string (may be null).
       * @return
       *   Pointer to the trimmed path inside the provided string, or the
       *   original pointer if no trim marker is found.
       */
      static CString TrimSourceFile(CString filePath);

      /**
       * Generates a formatted panic info string including file, line, and
       * function information.
       * @param file
       *   Source file name (may be null).
       * @param line
       *   Line number.
       * @param function
       *   Function name (may be null).
       * @return
       *   Formatted panic info string.
       */
      static CString GetPanicInfo(CString file, UInt32 line, CString function);
  };
}
