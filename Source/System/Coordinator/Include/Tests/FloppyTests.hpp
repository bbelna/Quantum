/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Include/Tests/FloppyTests.hpp
 * Coordinator floppy test suite.
 */

#pragma once

namespace Quantum::System::Coordinator::Tests {
  /**
   * Floppy device tests.
   */
  class FloppyTests {
    public:
      /**
       * Registers floppy tests with the coordinator harness.
       */
      static void RegisterTests();
  };
}
