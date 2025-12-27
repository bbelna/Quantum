/**
 * @file System/Kernel/TestRunner.cpp
 * @brief Kernel test runner task.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
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
