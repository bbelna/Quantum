/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Main.cpp
 * Driver entry trampoline.
 */

#include "Floppy.hpp"

extern "C" [[gnu::section(".text.start")]] void main() {
  Quantum::System::Drivers::Storage::Floppy::Main();
}
