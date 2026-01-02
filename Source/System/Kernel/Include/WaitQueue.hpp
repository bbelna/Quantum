/**
 * @file System/Kernel/Include/WaitQueue.hpp
 * @brief Simple wait queue for blocking threads.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "Thread.hpp"
#include "Sync/ScopedLock.hpp"
#include "Sync/SpinLock.hpp"

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
       * Enqueues the current thread and sleeps for up to the given ticks.
       * @param ticks
       *   Maximum number of ticks to wait.
       * @return
       *   True if woken by a signal; false if the wait timed out.
       */
      bool WaitTicks(UInt32 ticks);

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
      Sync::SpinLock _lock;
      Thread::ControlBlock* _head = nullptr;
      Thread::ControlBlock* _tail = nullptr;
  };
}
