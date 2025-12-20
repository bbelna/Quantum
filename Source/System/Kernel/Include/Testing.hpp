/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Tests.hpp
 * Kernel test harness for subsystem verification.
 */

#pragma once

#include <Types.hpp>
#include <String.hpp>

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
        CString Name;
        TestFunction Func;
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
      static TestCase _tests[_maxTests];

      /**
       * Number of registered tests.
       */
      static UInt32 _testCount;

      /**
       * Number of passed tests.
       */
      static UInt32 _testsPassed;

      /**
       * Number of failed tests.
       */
      static UInt32 _testsFailed;

      /**
       * Number of assertion failures recorded.
       */
      static UInt32 _assertFailures;

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

#define TEST_ASSERT(cond, msg) \
  ::Quantum::System::Kernel::Testing::Assert((cond), (msg), __FILE__, __LINE__)
