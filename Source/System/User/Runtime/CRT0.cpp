/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/User/Runtime/CRT0.cpp
 * User-mode CRT entry point.
 */

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

extern "C" int main();

extern "C" [[gnu::section(".text.start")]] [[noreturn]] void _start() {
  int code = main();

  Quantum::ABI::InvokeSystemCall(
    Quantum::ABI::SystemCall::Exit,
    static_cast<UInt32>(code)
  );

  for (;;) {
  }
}
