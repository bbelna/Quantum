/**
 * @file System/Kernel/Include/WaitQueue.hpp
 * @brief Simple wait queue for blocking threads.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "Thread.hpp"

namespace Quantum::System::Kernel {
  /**
   * FIFO wait queue for threads.
   */
  class WaitQueue {
    public:
      /**
       * Initializes the wait queue.
       */
      void Initialize();

      /**
       * Enqueues the current thread and yields.
       */
      void EnqueueCurrent();

      /**
       * Wakes a single thread from the queue.
       * @return
       *   True if a thread was woken.
       */
      bool WakeOne();

      /**
       * Wakes all threads from the queue.
       */
      void WakeAll();

    private:
      Thread::ControlBlock* _head = nullptr;
      Thread::ControlBlock* _tail = nullptr;
  };
}
