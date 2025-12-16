/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Tests.hpp
 * Kernel test harness for subsystem verification.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/String.hpp>

namespace Quantum::System::Kernel {
  /**
   * Signature for kernel test functions.
   */
  using TestFunc = bool (*)();

  /**
   * Test case descriptor.
   */
  struct TestCase {
    const char* Name;
    TestFunc Func;
  };

  /**
   * Kernel test harness.
   */
  class Tests {
    public:
      /**
       * Registers a test by name and function.
       * @param name
       *   Test name.
       * @param func
       *   Test function returning true on success.
       */
      static void Register(const char* name, TestFunc func);

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
  };
}

#define TEST_ASSERT(cond, msg) \
  ::Quantum::System::Kernel::Tests::Assert((cond), (msg), __FILE__, __LINE__)
