/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Task.hpp
 * IA32 task context and control structures.
 */

#pragma once

#include <Arch/IA32/Interrupts.hpp>
#include <Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * Low-level task management for IA32.
   */
  class Task {
    public:
      /**
       * Task context structure for IA32 architecture.
       */
      using Context = Interrupts::Context;

      /**
       * Task state enumeration.
       */
      enum State {
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
       * Task control block for IA32 architecture.
       */
      struct ControlBlock {
        /**
         * Unique task identifier.
         */
        UInt32 id;

        /**
         * Current task state.
         */
        State state;

        /**
         * Pointer to the saved interrupt context for the task.
         */
        Context* context;

        /**
         * Base address of the task's kernel stack.
         */
        void* stackBase;

        /**
         * Size of the task's kernel stack in bytes.
         */
        UInt32 stackSize;

        /**
         * Pointer to the next task in the scheduler queue.
         */
        ControlBlock* next;
      };

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
      static ControlBlock* Create(void (*entryPoint)(), UInt32 stackSize);

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
      static ControlBlock* GetCurrent();

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
       * @param context
       *   Current interrupt context.
       * @return
       *   Updated task context to switch to.
       */
      static Context* Tick(Context& context);
  };
}
