/**
 * @file System/Kernel/Include/Tests/MemoryTests.hpp
 * @brief Memory-related kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
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
