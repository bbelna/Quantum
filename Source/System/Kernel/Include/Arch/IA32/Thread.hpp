/**
 * @file System/Kernel/Include/Arch/IA32/Thread.hpp
 * @brief IA32 thread context and control structures.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Prelude.hpp>
#include <Types.hpp>

#include "Interrupts.hpp"

namespace Quantum::System::Kernel {
  struct TaskControlBlock;
}

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * Low-level thread management for IA32.
   */
  class Thread {
    public:
      /**
       * Thread context structure for IA32 architecture.
       */
      using Context = Interrupts::Context;

      /**
       * Thread state enumeration.
       */
      enum class State : UInt32 {
        /**
         * Thread is ready to run.
         */
        Ready = 0,

        /**
         * Thread is currently executing.
         */
        Running = 1,

        /**
         * Thread is blocked waiting for an event.
         */
        Blocked = 2,

        /**
         * Thread has terminated.
         */
        Terminated = 3
      };

      /**
       * Thread control block for IA32 architecture.
       */
      struct ControlBlock {
        /**
         * Unique thread identifier.
         */
        UInt32 id;

        /**
         * Owning task control block.
         */
        ::Quantum::System::Kernel::TaskControlBlock* task;

        /**
         * Current thread state.
         */
        State state;

        /**
         * Pointer to the saved interrupt context for the thread.
         */
        Context* context;

        /**
         * Base address of the thread's kernel stack.
         */
        void* stackBase;

        /**
         * Size of the thread's kernel stack in bytes.
         */
        UInt32 stackSize;

        /**
         * Top of the thread's kernel stack (stack grows downward).
         */
        UInt32 kernelStackTop;

        /**
         * User-mode entry point for user threads.
         */
        UInt32 userEntryPoint;

        /**
         * User-mode stack top for user threads.
         */
        UInt32 userStackTop;

        /**
         * Pointer to the next thread in the scheduler queue.
         */
        ControlBlock* next;

        /**
         * Pointer to the next thread in the global thread list.
         */
        ControlBlock* allNext;
      };

      /**
       * Initializes the IA32 thread subsystem.
       */
      static void Initialize();

      /**
       * Creates a new thread with the given entry point and stack size.
       * @param task
       *   Owning task control block.
       * @param entryPoint
       *   Function pointer to the thread's entry point.
       * @param stackSize
       *   Size of the thread's kernel stack in bytes.
       * @return
       *   Pointer to the thread control block, or nullptr on failure.
       */
      static ControlBlock* Create(
        ::Quantum::System::Kernel::TaskControlBlock* task,
        void (*entryPoint)(),
        UInt32 stackSize
      );

      /**
       * Creates a new user thread with the given entry point and stack.
       * @param task
       *   Owning task control block.
       * @param entryPoint
       *   User-mode entry point address.
       * @param userStackTop
       *   Top of the user-mode stack.
       * @param stackSize
       *   Size of the thread's kernel stack in bytes.
       * @return
       *   Pointer to the thread control block, or nullptr on failure.
       */
      static ControlBlock* CreateUser(
        ::Quantum::System::Kernel::TaskControlBlock* task,
        UInt32 entryPoint,
        UInt32 userStackTop,
        UInt32 stackSize
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
       * Finds a thread by id in the global thread list.
       * @param id
       *   Thread identifier to locate.
       * @return
       *   Pointer to the thread control block, or `nullptr` if not found.
       */
      static ControlBlock* FindById(UInt32 id);

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
       *   Updated thread context to switch to.
       */
      static Context* Tick(Context& context);

    private:
      /**
       * Pointer to the currently executing thread.
       */
      inline static ControlBlock* _currentThread = nullptr;

      /**
       * Pointer to the idle thread (never exits).
       */
      inline static ControlBlock* _idleThread = nullptr;

      /**
       * Head of the global thread list.
       */
      inline static ControlBlock* _allThreadsHead = nullptr;

      /**
       * Head of the ready queue.
       */
      inline static ControlBlock* _readyQueueHead = nullptr;

      /**
       * Tail of the ready queue.
       */
      inline static ControlBlock* _readyQueueTail = nullptr;

      /**
       * Thread pending cleanup (deferred until we are on a different stack).
       */
      inline static ControlBlock* _pendingCleanup = nullptr;

      /**
       * Whether preemptive scheduling is enabled.
       */
      inline static bool _preemptionEnabled = false;

      /**
       * When true, force a reschedule even if preemption is disabled.
       */
      inline static volatile bool _forceReschedule = false;

      /**
       * Becomes true after the first explicit yield to gate preemption.
       */
      inline static bool _schedulerActive = false;

      /**
       * Next thread ID to assign.
       */
      inline static UInt32 _nextThreadId = 1;

      /**
       * Adds a thread to the ready queue.
       */
      static void AddToReadyQueue(ControlBlock* thread);

      /**
       * Removes and returns the next thread from the ready queue.
       * @return
       *   Pointer to the next ready thread, or `nullptr` if none are ready.
       */
      static ControlBlock* PopFromReadyQueue();

      /**
       * Adds a thread to the global thread list.
       * @param thread
       *   Pointer to the thread to add.
       */
      static void AddToAllThreads(ControlBlock* thread);

      /**
       * Removes a thread from the global thread list.
       * @param thread
       *   Pointer to the thread to remove.
       */
      static void RemoveFromAllThreads(ControlBlock* thread);

      /**
       * Picks the next thread to run and returns its saved context pointer.
       * If `currentContext` is provided, saves it to the current TCB before
       * switching.
       * @param currentContext
       *   Pointer to the current thread's saved context, or `nullptr` if none.
       * @return
       *   Pointer to the next thread's saved context.
       */
      static Context* Schedule(Context* currentContext);

      /**
       * Idle thread entry point - runs when no other threads are ready.
       */
      static void IdleThread();

      /**
       * Thread wrapper that calls the actual entry point and exits cleanly.
       * @param entryPoint
       *   Function pointer to the thread's entry point.
       */
      static void ThreadWrapper(void (*entryPoint)());

      /**
       * User thread trampoline that enters user mode.
       */
      static void UserThreadTrampoline();

      /**
       * Creates a thread control block without enqueuing it.
       * @param task
       *   Owning task control block.
       * @param entryPoint
       *   Function pointer to the thread's entry point.
       * @param stackSize
       *   Size of the thread's kernel stack in bytes.
       * @return
       *   Pointer to the thread control block, or nullptr on failure.
       */
      static ControlBlock* CreateThreadInternal(
        ::Quantum::System::Kernel::TaskControlBlock* task,
        void (*entryPoint)(),
        UInt32 stackSize
      );
  };
}
