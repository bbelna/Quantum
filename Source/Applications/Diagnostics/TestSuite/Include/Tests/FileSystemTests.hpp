/**
 * @file Applications/Diagnostics/TestSuite/Include/Tests/FileSystemTests.hpp
 * @brief File system timeout tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  /**
   * File system tests.
   */
  class FileSystemTests {
    public:
      /**
       * Registers file system tests with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Tests request timeout usage in the file system ABI.
       * @return
       *   True on success.
       */
      static bool TestRequestTimeout();
  };
}
