/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/User/Runtime/CRT0.cpp
 * User-mode CRT entry point.
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
