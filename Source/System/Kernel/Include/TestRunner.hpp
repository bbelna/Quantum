/**
 * @file System/Kernel/Include/TestRunner.hpp
 * @brief Kernel test runner task.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
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
