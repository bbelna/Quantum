/**
 * @file System/Kernel/Testing.cpp
 * @brief Kernel testing framework.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Logger.hpp"
#include "Testing.hpp"
#include "Tests/MemoryTests.hpp"
#include "Tests/TaskTests.hpp"
#include "Tests/UserModeTests.hpp"

namespace Quantum::System::Kernel {
  using LogLevel = Logger::Level;

  void Testing::LogHeader() {
    Logger::Write(LogLevel::Info, "Running kernel test suite...");
  }

  void Testing::LogFooter() {
    Logger::WriteFormatted(
      _testsFailed == 0 ? LogLevel::Info : LogLevel::Error,
      "Kernel tests complete: passed=%x failed=%x total=%x",
      _testsPassed,
      _testsFailed,
      _testCount
    );
  }

  void Testing::Register(CString name, Testing::TestFunction func) {
    if (_testCount < _maxTests) {
      _tests[_testCount++] = Testing::TestCase { name, func };
    } else {
      Logger::Write(LogLevel::Error, "Test registry full");
    }
  }

  void Testing::Assert(
    bool condition,
    CString message,
    CString file,
    UInt32 line
  ) {
    if (!condition) {
      Logger::WriteFormatted(
        LogLevel::Error,
        "TEST ASSERT FAILED: %s (%s:%p)",
        message ? message : "unknown",
        file ? file : "unknown",
        line
      );

      _assertFailures++;
    }
  }

  void Testing::RunAll() {
    if (_testCount > 0) {
      LogHeader();

      for (UInt32 i = 0; i < _testCount; ++i) {
        Logger::WriteFormatted(LogLevel::Info, "[TEST] %s", _tests[i].name);

        UInt32 failedBefore = _assertFailures;
        bool ok = _tests[i].func ? _tests[i].func() : false;
        bool caseFailed = _assertFailures != failedBefore || !ok;

        if (!caseFailed) {
          _testsPassed++;
        } else {
          _testsFailed++;

          Logger::WriteFormatted(LogLevel::Error, "[FAIL] %s", _tests[i].name);
        }
      }

      LogFooter();
    } else {
      Logger::Write(LogLevel::Warning, "No kernel tests registered");
    }
  }

  UInt32 Testing::Passed() {
    return _testsPassed;
  }

  UInt32 Testing::Failed() {
    return _testsFailed;
  }

  void Testing::RegisterBuiltins() {
    Tests::MemoryTests::RegisterTests();
    Tests::TaskTests::RegisterTests();
    Tests::UserModeTests::RegisterTests();
  }
}
