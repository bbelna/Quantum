/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Task.hpp
 * IA32 task context and control structures.
 */

#pragma once

#include <Task.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  /**
   * IA32 CPU context saved during task switches.
   * Matches the layout expected by the context switch assembly routine.
   */
  struct TaskContext {
    UInt32 Edi;
    UInt32 Esi;
    UInt32 Ebx;
    UInt32 Ebp;
    UInt32 Eip;
  };

  /**
   * Task control block for IA32 architecture.
   */
  struct TaskControlBlock {
    /**
     * Unique task identifier.
     */
    UInt32 Id;

    /**
     * Current task state.
     */
    TaskState State;

    /**
     * Pointer to the saved stack pointer (ESP).
     * Points to a `TaskContext` structure on the stack.
     */
    TaskContext* StackPointer;

    /**
     * Base address of the task's kernel stack.
     */
    void* StackBase;

    /**
     * Size of the task's kernel stack in bytes.
     */
    UInt32 StackSize;

    /**
     * Pointer to the next task in the scheduler queue.
     */
    TaskControlBlock* Next;
  };

  /**
   * Low-level task management for IA32.
   */
  class Task {
    public:
      /**
       * Initializes the IA32 task subsystem.
       */
      static void Initialize();

      /**
       * Creates a new task with the given entry point and stack size.
       * @param entryPoint
       *   Function pointer to the task's entry point.
       * @param stackSize
       *   Size of the task's kernel stack in bytes.
       * @return
       *   Pointer to the task control block, or nullptr on failure.
       */
      static TaskControlBlock* Create(void (*entryPoint)(), UInt32 stackSize);

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
       * @return Pointer to the current task control block.
       */
      static TaskControlBlock* GetCurrent();

      /**
       * Enables preemptive multitasking.
       */
      static void EnablePreemption();

      /**
       * Disables preemptive multitasking.
       */
      static void DisablePreemption();

      /**
       * Scheduler tick handler.
       */
      static void Tick();

      /**
       * Performs a context switch from the current task to the next task.
       * @param currentStackPointer
       *   Pointer to store the current task's ESP.
       * @param nextStackPointer
       *   The next task's ESP to restore.
       */
      static void SwitchContext(
        TaskContext** currentStackPointer,
        TaskContext* nextStackPointer
      );
  };
}
