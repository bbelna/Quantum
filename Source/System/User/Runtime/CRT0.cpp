/**
 * @file System/User/Runtime/CRT0.cpp
 * @brief C runtime entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

int Main();

extern "C" [[gnu::section(".text.start")]] [[noreturn]] void Start() {
  int code = Main();

  Quantum::ABI::InvokeSystemCall(
    Quantum::ABI::SystemCall::Task_Exit,
    static_cast<UInt32>(code)
  );

  for (;;) {
  }
}
