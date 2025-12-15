//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Tests/TaskTests.hpp
// (c) 2025 Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Tasking test registrations.
//------------------------------------------------------------------------------

#pragma once

namespace Quantum::Kernel {
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
