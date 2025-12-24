/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Include/Tests/FAT12Tests.hpp
 * Coordinator FAT12 test suite.
 */

#pragma once

namespace Quantum::System::Coordinator::Tests {
  /**
   * FAT12 file system tests.
   */
  class FAT12Tests {
    public:
      /**
       * Registers FAT12 tests with the coordinator harness.
       */
      static void RegisterTests();
  };
}
