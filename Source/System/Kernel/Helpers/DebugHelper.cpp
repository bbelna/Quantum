/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Helpers/DebugHelper.hpp
 * Debug/diagnostic helper utilities.
 */

#include <Helpers/CStringHelper.hpp>
#include <Helpers/DebugHelper.hpp>

namespace Quantum::System::Kernel::Helpers {
  using Helpers::CStringHelper;

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

  CString DebugHelper::GetPanicInfo(
    CString file,
    UInt32 line,
    CString function
  ) {
    CString fileStr = file ? file : "unknown";
    CString funcStr = function ? function : "unknown";
    CString lineStr = nullptr;
    char lineBuffer[16] = {};

    if (
      line > 0 &&
      CStringHelper::ToCString(line, lineBuffer, sizeof(lineBuffer))
    ) {
      lineStr = lineBuffer;
    } else {
      lineStr = "unknown";
    }

    static char info[256];
    for (Size i = 0; i < sizeof(info); ++i) {
      info[i] = '\0';
    }
    Size out = 0;

    auto append = [&](CString src) -> bool {
      if (!src) {
        return true;
      } else {
        Size len = CStringHelper::Length(src);

        if (out + len >= sizeof(info)) {
          return false;
        } else {
          for (Size i = 0; i < len; ++i) {
            info[out++] = src[i];
          }

          return true;
        }
      }
    };

    CString trimmedFile = TrimSourceFile(fileStr);

    append("  ");
    append(trimmedFile);
    append(":");
    append(lineStr);
    append(" (");
    append(funcStr);
    append(")");

    info[out] = '\0';

    return info;
  }
}
