//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Task.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Architecture-agnostic task/thread management interface.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel {
  /**
   * Task state enumeration.
   */
  enum class TaskState : UInt8 {
    /**
     * Task is ready to run.
     */
    Ready = 0,

    /**
     * Task is currently executing.
     */
    Running = 1,

    /**
     * Task is blocked waiting for an event.
     */
    Blocked = 2,

    /**
     * Task has terminated.
     */
    Terminated = 3
  };

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
      static void Tick();
  };
}
