/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Task.hpp
 * Architecture-agnostic task/thread management interface.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/Interrupts/InterruptContext.hpp>
#include <Types/Tasks/TaskState.hpp>

namespace Quantum::System::Kernel {
  using Types::Interrupts::InterruptContext;
  using Types::Tasks::TaskState;

  /**
   * Forward declaration of architecture-specific task control block.
   */
  struct TaskControlBlock;

  /**
   * Task management and scheduling.
   */
  class Task {
    public:
      /**
       * Initializes the task subsystem and creates the idle task.
       */
      static void Initialize();

      /**
       * Creates a new kernel task.
       * @param entryPoint
       *   Function pointer to the task's entry point.
       * @param stackSize
       *   Size of the task's kernel stack in bytes.
       * @return
       *   Pointer to the task control block, or nullptr on failure.
       */
      static TaskControlBlock* Create(
        void (*entryPoint)(),
        UInt32 stackSize = 4096
      );

      /**
       * Terminates the current task.
       */
      [[noreturn]] static void Exit();

      /**
       * Yields the CPU to the next ready task.
       */
      static void Yield();

      /**
       * Gets the currently executing task.
       * @return
       *   Pointer to the current task control block.
       */
      static TaskControlBlock* GetCurrent();

      /**
       * Enables preemptive multitasking via timer interrupts.
       */
      static void EnablePreemption();

      /**
       * Disables preemptive multitasking.
       */
      static void DisablePreemption();

      /**
       * Scheduler tick handler (called from timer interrupt).
       */
      static InterruptContext* Tick(InterruptContext& context);
  };
}
