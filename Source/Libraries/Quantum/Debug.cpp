/**
 * @file Libraries/Quantum/Debug.cpp
 * @brief Debugging utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "CString.hpp"
#include "Debug.hpp"

namespace Quantum {
  CString TrimSourceFile(CString filePath) {
    if (!filePath) {
      return filePath;
    }

    CString trimmed = filePath;

    for (CString p = filePath; *p != '\0'; ++p) {
      bool separator = (p[0] == '/' || p[0] == '\\');

      if (
        separator
        && p[1] == 'S'
        && p[2] == 'o'
        && p[3] == 'u'
        && p[4] == 'r'
        && p[5] == 'c'
        && p[6] == 'e'
        && (p[7] == '/' || p[7] == '\\')
      ) {
        trimmed = p + 8;
      }
    }

    return trimmed;
  }
}
