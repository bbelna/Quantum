/**
 * @file Applications/Diagnostics/TestSuite/Include/Testing.hpp
 * @brief Test suite harness.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::Applications::Diagnostics::TestSuite {
  /**
   * Test suite harness for user-mode diagnostics.
   */
  class Testing {
    public:
      /**
       * Test case function signature.
       */
      using TestFunction = bool (*)();

      /**
       * Test case descriptor.
       */
      struct TestCase {
        /**
         * Test case name.
         */
        CString name;

        /**
         * Test case function.
         */
        TestFunction func;
      };

      /**
       * Registers a test case.
       * @param name
       *   Test name.
       * @param func
       *   Test function returning true on success.
       */
      static void Register(CString name, TestFunction func);

      /**
       * Logs an assertion failure.
       * @param condition
       *   Assertion condition.
       * @param message
       *   Assertion message.
       * @param file
       *   Source file name.
       * @param line
       *   Source line number.
       */
      static void Assert(
        bool condition,
        CString message,
        CString file,
        UInt32 line
      );

      /**
       * Runs all registered tests.
       */
      static void RunAll();

      /**
       * Registers built-in test groups.
       */
      static void RegisterBuiltins();

      /**
       * Returns the number of tests passed.
       * @return
       *   Number of tests passed.
       */
      static UInt32 Passed();

      /**
       * Returns the number of tests failed.
       * @return
       *   Number of tests failed.
       */
      static UInt32 Failed();

    private:
      /**
       * Logs the test run header.
       */
      static void LogHeader();

      /**
       * Logs the test run footer.
       */
      static void LogFooter();

      /**
       * Maximum number of tests supported.
       */
      static constexpr UInt32 _maxTests = 64;

      /**
       * Registered tests.
       */
      inline static TestCase _tests[_maxTests] = {};

      /**
       * Number of registered tests.
       */
      inline static UInt32 _testCount = 0;

      /**
       * Number of tests passed.
       */
      inline static UInt32 _testsPassed = 0;

      /**
       * Number of tests failed.
       */
      inline static UInt32 _testsFailed = 0;

      /**
       * Number of assertion failures.
       */
      inline static UInt32 _assertFailures = 0;
  };
}

#define TEST_ASSERT(cond, msg) \
  ((cond) ? true : ( \
    ::Quantum::Applications::Diagnostics::TestSuite::Testing::Assert( \
      false, \
      (msg), \
      __FILE__, \
      __LINE__ \
    ), \
    false \
  ))
