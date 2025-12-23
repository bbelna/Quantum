/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Testing.cpp
 * Coordinator test harness.
 */

#include <ABI/Console.hpp>

#include "Testing.hpp"
#include "Tests/FloppyTests.hpp"

namespace Quantum::System::Coordinator {
  using Console = ABI::Console;

  void Testing::WriteDec(UInt32 value) {
    char buffer[16] = {};
    UInt32 idx = 0;

    do {
      buffer[idx++] = static_cast<char>('0' + (value % 10));
      value /= 10;
    } while (value != 0 && idx < sizeof(buffer));

    while (idx > 0) {
      char c[2] = { buffer[--idx], '\0' };

      Console::Write(c);
    }
  }

  void Testing::LogHeader() {
    Console::WriteLine("Running coordinator test suite...");
  }

  void Testing::LogFooter() {
    Console::Write("Coordinator tests complete: passed=");
    WriteDec(_testsPassed);
    Console::Write(" failed=");
    WriteDec(_testsFailed);
    Console::Write(" total=");
    WriteDec(_testCount);
    Console::WriteLine("");
  }

  void Testing::Register(CString name, Testing::TestFunction func) {
    if (_testCount < _maxTests) {
      _tests[_testCount++] = Testing::TestCase{ name, func };
    } else {
      Console::WriteLine("Test registry full");
    }
  }

  void Testing::Assert(
    bool condition,
    CString message,
    CString file,
    UInt32 line
  ) {
    if (!condition) {
      Console::Write("TEST ASSERT FAILED: ");
      Console::Write(message ? message : "unknown");
      Console::Write(" (");
      Console::Write(file ? file : "unknown");
      Console::Write(":");
      WriteDec(line);
      Console::WriteLine(")");

      _assertFailures++;
    }
  }

  void Testing::RunAll() {
    if (_testCount > 0) {
      LogHeader();

      for (UInt32 i = 0; i < _testCount; ++i) {
        Console::Write("[TEST] ");
        Console::WriteLine(_tests[i].Name ? _tests[i].Name : "(unnamed)");

        UInt32 failedBefore = _assertFailures;
        bool ok = _tests[i].Func ? _tests[i].Func() : false;
        bool caseFailed = _assertFailures != failedBefore || !ok;

        if (!caseFailed) {
          _testsPassed++;
        } else {
          _testsFailed++;

          Console::Write("[FAIL] ");
          Console::WriteLine(_tests[i].Name ? _tests[i].Name : "(unnamed)");
        }
      }

      LogFooter();
    } else {
      Console::WriteLine("No coordinator tests registered");
    }
  }

  UInt32 Testing::Passed() {
    return _testsPassed;
  }

  UInt32 Testing::Failed() {
    return _testsFailed;
  }

  void Testing::RegisterBuiltins() {
    Tests::FloppyTests::RegisterTests();
  }
}
