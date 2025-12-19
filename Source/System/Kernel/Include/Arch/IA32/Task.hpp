/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Task.hpp
 * IA32 task context and control structures.
 */

#pragma once

#include <Prelude.hpp>
#include <Arch/IA32/Interrupts.hpp>
#include <Types/Tasks/TaskState.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using namespace Kernel::Types::Tasks;
  using namespace Types::Tasks;

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
       * Task control block for IA32 architecture.
       */
      struct ControlBlock {
        /**
         * Unique task identifier.
         */
        UInt32 Id;

        /**
         * Current task state.
         */
        TaskState State;

        /**
         * Pointer to the saved interrupt context for the task.
         */
        Context* SavedContext;

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
        ControlBlock* Next;
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
