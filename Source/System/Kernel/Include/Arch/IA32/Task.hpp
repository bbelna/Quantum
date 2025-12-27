/**
 * @file System/Kernel/Include/Arch/IA32/Task.hpp
 * @brief IA32 task context and control structures.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Prelude.hpp>
#include <Types.hpp>

#include "Interrupts.hpp"

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
      enum class State : UInt32 {
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
         * Capability flags granted to the task.
         */
        UInt32 caps;

        /**
         * Physical address of the task page directory.
         */
        UInt32 pageDirectoryPhysical;

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
         * Top of the task's kernel stack (stack grows downward).
         */
        UInt32 kernelStackTop;

        /**
         * User-mode entry point for user tasks.
         */
        UInt32 userEntryPoint;

        /**
         * User-mode stack top for user tasks.
         */
        UInt32 userStackTop;

        /**
         * User-mode heap base address.
         */
        UInt32 userHeapBase;

        /**
         * Current user-mode heap end (break).
         */
        UInt32 userHeapEnd;

        /**
         * End of the mapped heap region.
         */
        UInt32 userHeapMappedEnd;

        /**
         * User-mode heap upper limit.
         */
        UInt32 userHeapLimit;

        /**
         * Pointer to the next task in the scheduler queue.
         */
        ControlBlock* next;

        /**
         * Pointer to the next task in the global task list.
         */
        ControlBlock* allNext;
      };

      /**
       * Capability flags.
       */
      static constexpr UInt32 CapabilityIO = 1u << 0;

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
       * Creates a new user task with the given entry point and stack.
       * @param entryPoint
       *   User-mode entry point address.
       * @param userStackTop
       *   Top of the user-mode stack.
       * @param pageDirectoryPhysical
       *   Physical address of the user address space page directory.
       * @param stackSize
       *   Size of the task's kernel stack in bytes.
       * @return
       *   Pointer to the task control block, or nullptr on failure.
       */
      static ControlBlock* CreateUser(
        UInt32 entryPoint,
        UInt32 userStackTop,
        UInt32 pageDirectoryPhysical,
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
       * Finds a task by id in the global task list.
       * @param id
       *   Task identifier to locate.
       * @return
       *   Pointer to the task control block, or `nullptr` if not found.
       */
      static ControlBlock* FindById(UInt32 id);

      /**
       * Sets the address space for the current task.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to use.
       */
      static void SetCurrentAddressSpace(UInt32 pageDirectoryPhysical);

      /**
       * Gets the address space for the current task.
       * @return
       *   Physical address of the current task page directory.
       */
      static UInt32 GetCurrentAddressSpace();

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

      /**
       * Grants I/O access capability to the specified task.
       * @param taskId
       *   Task identifier to grant I/O access.
       * @return
       *   True if the capability was granted, false otherwise.
       */
      static bool GrantIOAccess(UInt32 taskId);

      /**
       * Checks if the current task has I/O access capability.
       * @return
       *   True if the current task has I/O access, false otherwise.
       */
      static bool CurrentTaskHasIOAccess();

    private:
      /**
       * Pointer to the currently executing task.
       */
      inline static ControlBlock* _currentTask = nullptr;

      /**
       * Pointer to the idle task (never exits).
       */
      inline static ControlBlock* _idleTask = nullptr;

      /**
       * Head of the global task list.
       */
      inline static ControlBlock* _allTasksHead = nullptr;

      /**
       * Head of the ready queue.
       */
      inline static ControlBlock* _readyQueueHead = nullptr;

      /**
       * Tail of the ready queue.
       */
      inline static ControlBlock* _readyQueueTail = nullptr;

      /**
       * Task pending cleanup (deferred until we are on a different stack).
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
       * Next task ID to assign.
       */
      inline static UInt32 _nextTaskId = 1;

      /**
       * Adds a task to the ready queue.
       */
      static void AddToReadyQueue(ControlBlock* task);

      /**
       * Removes and returns the next task from the ready queue.
       * @return
       *   Pointer to the next ready task, or `nullptr` if none are ready.
       */
      static ControlBlock* PopFromReadyQueue();

      /**
       * Adds a task to the global task list.
       * @param task
       *   Pointer to the task to add.
       */
      static void AddToAllTasks(ControlBlock* task);

      /**
       * Removes a task from the global task list.
       * @param task
       *   Pointer to the task to remove.
       */
      static void RemoveFromAllTasks(ControlBlock* task);

      /**
       * Picks the next task to run and returns its saved context pointer.
       * If `currentContext` is provided, saves it to the current TCB before
       * switching.
       * @param currentContext
       *   Pointer to the current task's saved context, or `nullptr` if none.
       * @return
       *   Pointer to the next task's saved context.
       */
      static Context* Schedule(Context* currentContext);

      /**
       * Idle task entry point - runs when no other tasks are ready.
       */
      static void IdleTask();

      /**
       * Task wrapper that calls the actual entry point and exits cleanly.
       * @param entryPoint
       *   Function pointer to the task's entry point.
       */
      static void TaskWrapper(void (*entryPoint)());

      /**
       * User task trampoline that enters user mode.
       */
      static void UserTaskTrampoline();

      /**
       * Creates a task control block without enqueuing it.
       * @param entryPoint
       *   Function pointer to the task's entry point.
       * @param stackSize
       *   Size of the task's kernel stack in bytes.
       * @return
       *   Pointer to the task control block, or nullptr on failure.
       */
      static ControlBlock* CreateTaskInternal(
        void (*entryPoint)(),
        UInt32 stackSize
      );
  };
}
