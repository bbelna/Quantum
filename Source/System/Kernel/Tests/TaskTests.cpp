/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Tests/TaskTests.cpp
 * Tasking tests.
 */

#include <Task.hpp>
#include <Tests.hpp>
#include <Tests/TaskTests.hpp>

namespace Quantum::System::Kernel {
  namespace {
    /**
     * Shared counter incremented by cooperating tasks.
     */
    volatile UInt32 taskCounter = 0;

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
     * Verifies cooperative yields between two tasks.
     * @return True if the test passes.
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
  }

  void TaskTests::RegisterTests() {
    Tests::Register("Task yield scheduling", TestTaskYield);
  }
}
