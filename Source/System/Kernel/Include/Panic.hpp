/**
 * @file System/Kernel/Include/Panic.hpp
 * @brief Panic handling utilities.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#define PANIC(msg) ::Quantum::System::Kernel::Panic(\
  (msg), __FILE__, __LINE__, __FUNCTION__\
)

namespace Quantum::System::Kernel {
  /**
   * Handle a kernel panic. Do not invoke directly; use the `PANIC` macro.
   * @param message
   *   Panic message.
   * @param file
   *   The source file where the panic occurred (optional).
   * @param line
   *   The line number where the panic occurred (optional).
   * @param function
   *   The function name where the panic occurred (optional).
   */
  [[noreturn]]
  void Panic(
    CString message,
    CString file = nullptr,
    UInt32 line = -1,
    CString function = nullptr
  );
}
