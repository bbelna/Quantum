/**
 * @file System/Kernel/Include/Tests/TaskTests.hpp
 * @brief Tasking-related kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace Quantum::System::Kernel::Tests {
  /**
   * Registers tasking-related kernel tests.
   */
  class TaskTests {
    public:
      /**
       * Registers tasking test cases with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Flag to stop long-running spinner tasks used for preemption testing.
       */
      inline static volatile bool _stopSpinTasks = false;

      /**
       * Shared counter incremented by cooperating tasks.
       */
      inline static volatile UInt32 _taskCounter = 0;

      /**
       * Counter for first preemptive spinner task.
       */
      inline static volatile UInt32 _preemptCounterA = 0;

      /**
       * Counter for second preemptive spinner task.
       */
      inline static volatile UInt32 _preemptCounterB = 0;

      /**
       * First cooperating task increments shared counter and yields.
       */
      static void TaskA();

      /**
       * Second cooperating task increments shared counter and yields.
       */
      static void TaskB();

      /**
       * Spinner task used to verify preemption. Never yields; increments its
       * own counter until asked to stop.
       */
      static void PreemptTaskA();

      /**
       * Second spinner task for preemption testing.
       */
      static void PreemptTaskB();

      /**
       * Verifies cooperative yields between two tasks.
       * @return
       *   True if the test passes.
       */
      static bool TestTaskYield();

      /**
       * Verifies that timer-driven preemption switches between busy tasks even
       * without cooperative yields.
       * @return
       *   True if the test passes.
       */
      static bool TestTaskPreemption();
  };
}
