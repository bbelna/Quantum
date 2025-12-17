/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Tests/TaskTests.cpp
 * Tasking tests.
 */

#include <Task.hpp>
#include <Testing.hpp>
#include <Tests/TaskTests.hpp>

namespace Quantum::System::Kernel::Tests {
  namespace {
    /**
     * Flag to stop long-running spinner tasks used for preemption testing.
     */
    volatile bool stopSpinTasks = false;

    /**
     * Shared counter incremented by cooperating tasks.
     */
    volatile UInt32 taskCounter = 0;

    /**
     * Counter for first preemptive spinner task.
     */
    volatile UInt32 preemptCounterA = 0;

    /**
     * Counter for second preemptive spinner task.
     */
    volatile UInt32 preemptCounterB = 0;

    /**
     * First cooperating task increments shared counter and yields.
     */
    void TaskA() {
      taskCounter += 1;

      Task::Yield();

      taskCounter += 1;

      Task::Exit();
    }

    /**
     * Second cooperating task increments shared counter and yields.
     */
    void TaskB() {
      taskCounter += 1;

      Task::Yield();

      taskCounter += 1;

      Task::Exit();
    }

    /**
     * Spinner task used to verify preemption. Never yields; increments its own
     * counter until asked to stop.
     */
    void PreemptTaskA() {
      while (!stopSpinTasks) {
        preemptCounterA++;
      }

      Task::Exit();
    }

    /**
     * Second spinner task for preemption testing.
     */
    void PreemptTaskB() {
      while (!stopSpinTasks) {
        preemptCounterB++;
      }

      Task::Exit();
    }

    /**
     * Verifies cooperative yields between two tasks.
     * @return
     *   True if the test passes.
     */
    bool TestTaskYield() {
      taskCounter = 0;

      Task::Create(TaskA, 4096);
      Task::Create(TaskB, 4096);

      // Yield until both tasks have run to completion.
      while (taskCounter < 4) {
        Task::Yield();
      }

      TEST_ASSERT(taskCounter == 4, "Expected 4 increments across tasks");

      return true;
    }

    /**
     * Verifies that timer-driven preemption switches between busy tasks even
     * without cooperative yields.
     * @return
     *   True if the test passes.
     */
    bool TestTaskPreemption() {
      preemptCounterA = 0;
      preemptCounterB = 0;
      stopSpinTasks = false;

      Task::EnablePreemption();

      Task::Create(PreemptTaskA, 4096);
      Task::Create(PreemptTaskB, 4096);

      const UInt32 target = 500;
      const UInt32 maxIterations = 50000000;
      UInt32 iterations = 0;

      // busy-wait without yielding; only preemption should advance counters
      while (
        (preemptCounterA < target || preemptCounterB < target) &&
        iterations < maxIterations
      ) {
        iterations++;
      }

      stopSpinTasks = true;

      // give spinner tasks a chance to see the stop flag and exits
      for (int i = 0; i < 4; ++i) {
        Task::Yield();
      }

      TEST_ASSERT(
        preemptCounterA >= target && preemptCounterB >= target,
        "Preemption did not advance both tasks"
      );

      return true;
    }
  }

  void TaskTests::RegisterTests() {
    Testing::Register("Task yield scheduling", TestTaskYield);
    Testing::Register("Task preemption scheduling", TestTaskPreemption);
  }
}
