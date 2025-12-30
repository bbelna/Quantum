/**
 * @file System/Kernel/Include/Tests/IPCTests.hpp
 * @brief IPC-related kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

namespace Quantum::System::Kernel::Tests {
  /**
   * Registers IPC-related kernel tests.
   */
  class IPCTests {
    public:
      /**
       * Registers IPC test cases with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Indicates send completion for send/receive test.
       */
      inline static volatile bool _sendDone = false;

      /**
       * Indicates receive completion for send/receive test.
       */
      inline static volatile bool _recvDone = false;

      /**
       * Indicates receive success for send/receive test.
       */
      inline static volatile bool _recvOk = false;

      /**
       * Received payload length for send/receive test.
       */
      inline static volatile UInt32 _recvLength = 0;

      /**
       * Port identifier for send/receive test.
       */
      inline static UInt32 _portId = 0;

      /**
       * Receive buffer for send/receive test.
       */
      inline static UInt8 _recvBuffer[16] = {};

      /**
       * Task function that sends a message.
       */
      static void SenderTask();

      /**
       * Task function that receives a message.
       */
      static void ReceiverTask();

      /**
       * Tests sending and receiving a message.
       * @return
       *   True on success; false on failure.
       */
      static bool TestSendReceive();
  };
}
