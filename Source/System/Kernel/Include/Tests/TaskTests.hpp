/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Tests/TaskTests.hpp
 * Task subsystem tests.
 */

#pragma once

namespace Quantum::System::Kernel::Tests {
  /**
   * Registers tasking-related kernel tests.
   */
  class TaskTests {
    public:
      /**
       * Registers tasking test cases with the harness.
       */
      static void RegisterTests();
  };
}
