/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Tests/TaskTests.cpp
 * Tasking tests.
 */

#include "Macros.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "Tests/TaskTests.hpp"

namespace Quantum::System::Kernel::Tests {
  void TaskTests::TaskA() {
    _taskCounter += 1;

    Task::Yield();

    _taskCounter += 1;

    Task::Exit();
  }

  void TaskTests::TaskB() {
    _taskCounter += 1;

    Task::Yield();

    _taskCounter += 1;

    Task::Exit();
  }

  void TaskTests::PreemptTaskA() {
    while (!_stopSpinTasks) {
      _preemptCounterA++;
    }

    Task::Exit();
  }

  void TaskTests::PreemptTaskB() {
    while (!_stopSpinTasks) {
      _preemptCounterB++;
    }

    Task::Exit();
  }

  bool TaskTests::TestTaskYield() {
    _taskCounter = 0;

    Task::Create(TaskA, 4096);
    Task::Create(TaskB, 4096);

    // yield until both tasks have run to completion
    while (_taskCounter < 4) {
      Task::Yield();
    }

    TEST_ASSERT(_taskCounter == 4, "Expected 4 increments across tasks");

    return true;
  }

  bool TaskTests::TestTaskPreemption() {
    _preemptCounterA = 0;
    _preemptCounterB = 0;
    _stopSpinTasks = false;

    Task::EnablePreemption();

    Task::Create(PreemptTaskA, 4096);
    Task::Create(PreemptTaskB, 4096);

    const UInt32 target = 500;
    const UInt32 maxIterations = 50000000;
    UInt32 iterations = 0;

    // busy-wait without yielding; only preemption should advance counters
    while (
      (_preemptCounterA < target || _preemptCounterB < target) &&
      iterations < maxIterations
    ) {
      iterations++;
    }

    _stopSpinTasks = true;

    // give spinner tasks a chance to see the stop flag and exits
    for (int i = 0; i < 4; ++i) {
      Task::Yield();
    }

    TEST_ASSERT(
      _preemptCounterA >= target && _preemptCounterB >= target,
      "Preemption did not advance both tasks"
    );

    return true;
  }

  void TaskTests::RegisterTests() {
    Testing::Register("Task yield scheduling", TestTaskYield);
    Testing::Register("Task preemption scheduling", TestTaskPreemption);
  }
}
