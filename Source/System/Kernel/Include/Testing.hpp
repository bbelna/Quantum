/**
 * @file System/Kernel/Include/Testing.hpp
 * @brief Kernel testing framework.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#define TEST_ASSERT(cond, msg) \
  ::Quantum::System::Kernel::Testing::Assert((cond), (msg), __FILE__, __LINE__)

namespace Quantum::System::Kernel {
  /**
   * Kernel test harness.
   */
  class Testing {
    public:
      /**
       * Signature for kernel test functions.
       */
      typedef bool (*TestFunction)();

      /**
       * Test case descriptor.
       */
      struct TestCase {
        CString name;
        TestFunction func;
      };

      /**
       * Registers a test by name and function.
       * @param name
       *   Test name.
       * @param func
       *   Test function returning true on success.
       */
      static void Register(CString name, TestFunction func);

      /**
       * Runs all registered tests and logs results.
       */
      static void RunAll();

      /**
       * Records a failed assertion.
       * @param condition
       *   Condition to verify.
       * @param message
       *   Description of the assertion.
       * @param file
       *   Source file of the assertion.
       * @param line
       *   Line number of the assertion.
       */
      static void Assert(
        bool condition,
        CString message,
        CString file,
        UInt32 line
      );

      /**
       * Returns the number of passed tests.
       * @return
       *   Number of passed tests.
       */
      static UInt32 Passed();

      /**
       * Returns the number of failed tests.
       * @return
       *   Number of failed tests.
       */
      static UInt32 Failed();

      /**
       * Registers built-in test suites.
       */
      static void RegisterBuiltins();

    private:
      /**
       * Maximum number of registered tests.
       */
      static constexpr UInt32 _maxTests = 32;

      /**
       * Registered test cases.
       */
      inline static TestCase _tests[_maxTests];

      /**
       * Number of registered tests.
       */
      inline static UInt32 _testCount = 0;

      /**
       * Number of passed tests.
       */
      inline static UInt32 _testsPassed = 0;

      /**
       * Number of failed tests.
       */
      inline static UInt32 _testsFailed = 0;

      /**
       * Number of assertion failures recorded.
       */
      inline static UInt32 _assertFailures = 0;

      /**
       * Logs the header before running tests.
       */
      static void LogHeader();

      /**
       * Logs the footer after running tests.
       */
      static void LogFooter();
  };
}
