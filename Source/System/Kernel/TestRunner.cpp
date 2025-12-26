/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/TestRunner.cpp
 * Kernel test runner for running kernel tests.
 */

#include "Logger.hpp"
#include "Prelude.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "TestRunner.hpp"

namespace Quantum::System::Kernel {
  using LogLevel = Kernel::Logger::Level;

  void TestRunner::Run() {
    Testing::RegisterBuiltins();
    Testing::RunAll();
    Logger::Write(LogLevel::Info, "Kernel tests task finished");
    Task::Exit();
  }
}
