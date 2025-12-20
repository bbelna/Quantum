/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.cpp
 * User-mode floppy driver entry.
 */

#include <Console.hpp>
#include <Task.hpp>

#include "Driver.hpp"

namespace Quantum::System::Drivers::Storage::Floppy {
  void Driver::Main() {
    Console::WriteLine("Floppy driver starting (stub)");

    // TODO: replace with real driver initialization and event loop.

    Task::Exit(0);
  }
}
