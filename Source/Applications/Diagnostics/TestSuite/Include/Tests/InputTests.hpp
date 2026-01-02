/**
 * @file Applications/Diagnostics/TestSuite/Include/Tests/InputTests.hpp
 * @brief Input device tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  /**
   * Input device tests.
   */
  class InputTests {
    public:
      /**
       * Registers input tests with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Indicates whether a skip reason has been logged.
       */
      inline static bool _skipLogged = false;

      /**
       * Logs a skip reason.
       * @param reason
       *   Skip reason.
       */
      static void LogSkip(CString reason);

      /**
       * Tests for the presence of a keyboard device.
       * @return
       *   True if a keyboard device is present.
       */
      static bool TestKeyboardPresent();

      /**
       * Tests receiving a keyboard event.
       * @return
       *   True on success.
       */
      static bool TestKeyboardEvent();

      /**
       * Tests broker subscribe/unsubscribe with a timeout.
       * @return
       *   True on success.
       */
      static bool TestSubscribeTimeout();

      /**
       * Tests input device read timeout handling.
       * @return
       *   True on success.
       */
      static bool TestReadTimeout();
  };
}
