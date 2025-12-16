//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Helpers/DebugHelper.cpp
// (c) 2025 Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Debug/diagnostic helper utilities.
//------------------------------------------------------------------------------

#include <Helpers/DebugHelper.hpp>

namespace Quantum::Kernel::Helpers {
  CString DebugHelper::TrimSourceFile(CString filePath) {
    if (!filePath) {
      return filePath;
    }

    CString trimmed = filePath;

    for (CString p = filePath; *p != '\0'; ++p) {
      bool separator = (p[0] == '/' || p[0] == '\\');

      if (
        separator &&
        p[1] == 'S' &&
        p[2] == 'o' &&
        p[3] == 'u' &&
        p[4] == 'r' &&
        p[5] == 'c' &&
        p[6] == 'e' &&
        (p[7] == '/' || p[7] == '\\')
      ) {
        trimmed = p + 8;
      }
    }

    return trimmed;
  }
}
