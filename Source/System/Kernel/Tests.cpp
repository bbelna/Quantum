//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Tests.cpp
// (c) 2025 Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Test harness for subsystem verification.
//------------------------------------------------------------------------------

#include <Logger.hpp>
#include <Tests.hpp>
#include <Types.hpp>
#include <Tests/MemoryTests.hpp>
#include <Tests/TaskTests.hpp>

namespace Quantum::Kernel {
  using LogLevel = Logger::Level;

  namespace {
    /**
     * Maximum number of registered tests.
     */
    constexpr UInt32 _maxTests = 32;

    /**
     * Registered test cases.
     */
    TestCase _tests[_maxTests];

    /**
     * Number of registered tests.
     */
    UInt32 _testCount = 0;

    /**
     * Number of passed tests.
     */
    UInt32 _testsPassed = 0;

    /**
     * Number of failed tests.
     */
    UInt32 _testsFailed = 0;

    /**
     * Number of assertion failures recorded.
     */
    UInt32 _assertFailures = 0;

    /**
     * Logs the header before running tests.
     */
    void LogHeader() {
      Logger::Write(LogLevel::Info, "Running kernel test suite...");
    }

    /**
     * Logs the footer after running tests.
     */
    void LogFooter() {
      Logger::WriteFormatted(
        _testsFailed == 0 ? LogLevel::Info : LogLevel::Error,
        "Kernel tests complete: passed=%p failed=%p total=%p",
        _testsPassed,
        _testsFailed,
        _testCount
      );
    }
  }

  void Tests::Register(CString name, TestFunc func) {
    if (_testCount < _maxTests) {
      _tests[_testCount++] = TestCase{ name, func };
    } else {
      Logger::Write(LogLevel::Error, "Test registry full");
    }
  }

  void Tests::Assert(
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

  void Tests::RunAll() {
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

  UInt32 Tests::Passed() {
    return _testsPassed;
  }

  UInt32 Tests::Failed() {
    return _testsFailed;
  }

  void Tests::RegisterBuiltins() {
    MemoryTests::RegisterTests();
    TaskTests::RegisterTests();
  }
}
