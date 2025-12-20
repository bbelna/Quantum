/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Tests.cpp
 * Test harness for subsystem verification.
 */

#include <Logger.hpp>
#include <Testing.hpp>
#include <Tests/MemoryTests.hpp>
#include <Tests/TaskTests.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel {
  using namespace Tests;

  using LogLevel = Logger::Level;

  Testing::TestCase Testing::_tests[Testing::_maxTests];
  UInt32 Testing::_testCount = 0;
  UInt32 Testing::_testsPassed = 0;
  UInt32 Testing::_testsFailed = 0;
  UInt32 Testing::_assertFailures = 0;

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
      _tests[_testCount++] = Testing::TestCase{ name, func };
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
        Logger::WriteFormatted(LogLevel::Info, "[TEST] %s", _tests[i].Name);

        UInt32 failedBefore = _assertFailures;
        bool ok = _tests[i].Func ? _tests[i].Func() : false;
        bool caseFailed = _assertFailures != failedBefore || !ok;

        if (!caseFailed) {
          _testsPassed++;
        } else {
          _testsFailed++;

          Logger::WriteFormatted(LogLevel::Error, "[FAIL] %s", _tests[i].Name);
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
    MemoryTests::RegisterTests();
    TaskTests::RegisterTests();
  }
}
