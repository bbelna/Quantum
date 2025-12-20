/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Tests/MemoryTests.hpp
 * Memory subsystem tests.
 */

#pragma once

namespace Quantum::System::Kernel::Tests {
  /**
   * Registers memory-related kernel tests.
   */
  class MemoryTests {
    public:
      /**
       * Registers memory test cases with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Verifies basic allocate/free round-trip.
       * @return
       *   True if the test passes.
       */
      static bool TestMemoryAllocation();
  };
}
