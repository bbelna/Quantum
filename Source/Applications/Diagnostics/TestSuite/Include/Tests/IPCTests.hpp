/**
 * @file Applications/Diagnostics/TestSuite/Include/Tests/IPCTests.hpp
 * @brief IPC tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  /**
   * IPC tests.
   */
  class IPCTests {
    public:
      /**
       * Registers IPC tests with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Tests an IPC send/receive loopback path.
       * @return
       *   True on success.
       */
      static bool TestLoopback();

      /**
       * Tests handle transfer over IPC.
       * @return
       *   True on success.
       */
      static bool TestHandleTransfer();

      /**
       * Tests IPC receive timeout behavior.
       * @return
       *   True on success.
       */
      static bool TestReceiveTimeout();
  };
}
