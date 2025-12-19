/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Task.hpp
 * Architecture-agnostic task/thread management interface.
 */

#pragma once

#include <Types.hpp>
#include <Interrupts.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/Task.hpp>
#endif

namespace Quantum::System::Kernel {
  /**
   * Task management and scheduling.
   */
  class Task {
    public:
      #if defined(QUANTUM_ARCH_IA32)
      using ControlBlock = Arch::IA32::Task::ControlBlock;
      #else
      using ControlBlock = void;
      #endif

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
      static ControlBlock* Create(
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
      static ControlBlock* GetCurrent();

      /**
       * Gets the task id of the currently executing task (0 if none).
       */
      static UInt32 GetCurrentId();

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
      static Interrupts::Context* Tick(Interrupts::Context& context);
  };
}
