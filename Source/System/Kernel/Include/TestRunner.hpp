/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/TestRunner.hpp
 * Kernel test runner for running kernel tests.
 */

#pragma once

namespace Quantum::System::Kernel {
  /**
   * The kernel test runner.
   */
  class TestRunner {
    public:
      /**
       * Runs all registered kernel tests.
       */
      static void Run();
  };
}
