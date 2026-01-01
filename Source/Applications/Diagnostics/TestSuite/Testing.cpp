/**
 * @file Applications/Diagnostics/TestSuite/Testing.cpp
 * @brief Test suite harness.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <Debug.hpp>

#include "Testing.hpp"
#include "Tests/FAT12Tests.hpp"
#include "Tests/FloppyTests.hpp"
#include "Tests/IPCTests.hpp"
#include "Tests/InputTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite {
  using ABI::Console;

  static void WriteDec(UInt32 value) {
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
    Console::WriteLine("Running TestSuite...");
  }

  void Testing::LogFooter() {
    Console::Write("TestSuite complete: passed=");
    WriteDec(_testsPassed);
    Console::Write(" failed=");
    WriteDec(_testsFailed);
    Console::Write(" total=");
    WriteDec(_testCount);
    Console::WriteLine("");
  }

  void Testing::Register(CString name, Testing::TestFunction func) {
    if (_testCount < _maxTests) {
      _tests[_testCount++] = Testing::TestCase { name, func };
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
      Console::WriteLine("TEST ASSERT FAILED :(");
      Console::Write("  ");
      Console::WriteLine(message ? message : "unknown");
      Console::Write("  ");
      Console::Write(file ? TrimSourceFile(file) : "unknown");
      Console::Write(" : ");
      WriteDec(line);
      Console::WriteLine("");

      _assertFailures++;
    }
  }

  void Testing::RunAll() {
    if (_testCount == 0) {
      Console::WriteLine("No tests registered");

      return;
    }

    LogHeader();

    for (UInt32 i = 0; i < _testCount; ++i) {
      Console::Write("[TEST] ");
      Console::WriteLine(_tests[i].name ? _tests[i].name : "(unnamed)");

      UInt32 failedBefore = _assertFailures;
      bool ok = _tests[i].func ? _tests[i].func() : false;
      bool caseFailed = _assertFailures != failedBefore || !ok;

      if (!caseFailed) {
        _testsPassed++;
      } else {
        _testsFailed++;

        Console::Write("[FAIL] ");
        Console::WriteLine(_tests[i].name ? _tests[i].name : "(unnamed)");
      }
    }

    LogFooter();
  }

  UInt32 Testing::Passed() {
    return _testsPassed;
  }

  UInt32 Testing::Failed() {
    return _testsFailed;
  }

  void Testing::RegisterBuiltins() {
    Tests::FloppyTests::RegisterTests();
    Tests::FAT12Tests::RegisterTests();
    Tests::InputTests::RegisterTests();
    Tests::IPCTests::RegisterTests();
  }
}
