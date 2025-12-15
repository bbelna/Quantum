//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Tests/MemoryTests.hpp
// (c) 2025 Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Memory subsystem test registrations.
//------------------------------------------------------------------------------

#pragma once

namespace Quantum::Kernel {
  /**
   * Registers memory-related kernel tests.
   */
  class MemoryTests {
    public:
      /**
       * Registers memory test cases with the harness.
       */
      static void RegisterTests();
  };
}
