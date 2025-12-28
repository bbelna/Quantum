/**
 * @file System/User/Runtime/CRT0.cpp
 * @brief C runtime entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

/**
 * User mode application entry point.
 * @return
 *   Exit code.
 */
int Main();

/**
 * C runtime start function.
 * Invokes the application entry point and exits via system call.
 */
extern "C" [[gnu::section(".text.start")]] [[noreturn]] void Start() {
  int code = Main();

  Quantum::ABI::InvokeSystemCall(
    Quantum::ABI::SystemCall::Task_Exit,
    static_cast<UInt32>(code)
  );

  for (;;) {
  }
}
