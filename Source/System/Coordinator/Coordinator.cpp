/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Coordinator.cpp
 * System coordinator implementation.
 */

#include <Console.hpp>
#include <Task.hpp>

#include "Coordinator.hpp"

namespace Quantum::System::Coordinator {
  void Main() {
    Console::WriteLine("Coordinator");
    Task::Exit(1);
  }
}

extern "C" [[gnu::section(".text.start")]] void _start() {
  Quantum::System::Coordinator::Main();
  Quantum::Task::Exit(0);

  for (;;) {
    // do nothing
  }
}
