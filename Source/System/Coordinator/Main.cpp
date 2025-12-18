/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Main.cpp
 * Coordinator main entry point.
 */

#include "Coordinator.hpp"

/**
 * Coordinator main entry point.
 */
extern "C" [[gnu::section(".text.start")]] void main() {
  Quantum::System::Coordinator::Main();
}
