/**
 * @file Libraries/Quantum/Include/Debug.hpp
 * @brief Debugging utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Types.hpp"

namespace Quantum {
  /**
   * Trims a fully qualified source file path to a shorter form by removing
   * any prefix up to and including "Source".
   * @param filePath
   *   Full path string (may be null).
   * @return
   *   Pointer to the trimmed path inside the provided string, or the
   *   original pointer if no trim marker is found.
   */
  CString TrimSourceFile(CString filePath);
}
