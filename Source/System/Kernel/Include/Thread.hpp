/**
 * @file System/Kernel/Include/Thread.hpp
 * @brief Architecture-agnostic thread management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Arch/Thread.hpp"
#include "Interrupts.hpp"

namespace Quantum::System::Kernel {
  struct TaskControlBlock;

  /**
   * Thread management and scheduling.
   */
  class Thread {
    public:
      using ControlBlock = Arch::Thread::ControlBlock;
      using State = Arch::Thread::State;

      /**
       * Initializes the thread scheduler and creates the idle thread.
       */
      static void Initialize();

      /**
       * Creates a new kernel thread bound to a task.
       * @param task
       *   Owning task control block.
       * @param entryPoint
       *   Function pointer to the thread's entry point.
       * @param stackSize
       *   Size of the thread's kernel stack in bytes.
       * @return
       *   Pointer to the thread control block, or `nullptr` on failure.
       */
      static ControlBlock* Create(
        TaskControlBlock* task,
        void (*entryPoint)(),
        UInt32 stackSize = 4096
      );

      /**
       * Creates a new user thread bound to a task.
       * @param task
       *   Owning task control block.
       * @param entryPoint
       *   User-mode entry point address.
       * @param userStackTop
       *   Top of the user-mode stack.
       * @param stackSize
       *   Size of the thread's kernel stack in bytes.
       * @return
       *   Pointer to the thread control block, or `nullptr` on failure.
       */
      static ControlBlock* CreateUser(
        TaskControlBlock* task,
        UInt32 entryPoint,
        UInt32 userStackTop,
        UInt32 stackSize = 4096
      );

      /**
       * Terminates the current thread.
       */
      [[noreturn]] static void Exit();

      /**
       * Yields the CPU to the next ready thread.
       */
      static void Yield();

      /**
       * Gets the currently executing thread.
       * @return
       *   Pointer to the current thread control block.
       */
      static ControlBlock* GetCurrent();

      /**
       * Gets the thread id of the currently executing thread (0 if none).
       * @return
       *   Thread identifier.
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
       * @param context
       *   Reference to the current interrupt context.
       * @return
       *   Pointer to the context to resume after scheduling.
       */
      static Interrupts::Context* Tick(Interrupts::Context& context);

      /**
       * Marks a blocked thread as ready and enqueues it.
       * @param thread
       *   Thread to wake.
       */
      static void Wake(ControlBlock* thread);

      /**
       * Sleeps the current thread for the specified number of ticks.
       * @param ticks
       *   Number of timer ticks to sleep.
       */
      static void SleepTicks(UInt32 ticks);
  };
}
