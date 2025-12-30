/**
 * @file System/Kernel/Include/Task.hpp
 * @brief Architecture-agnostic task (process) management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Interrupts.hpp"
#include "Thread.hpp"

namespace Quantum::System::Kernel {
  class HandleTable;

  /**
   * Task control block.
   */
  struct TaskControlBlock {
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
     * Per-task handle table.
     */
    HandleTable* handleTable;

    /**
     * Primary thread for this task.
     */
    Thread::ControlBlock* mainThread;

    /**
     * Head of the task's thread list.
     */
    Thread::ControlBlock* threadHead;

    /**
     * Number of threads owned by this task.
     */
    UInt32 threadCount;

    /**
     * Pointer to the next task in the global task list.
     */
    TaskControlBlock* next;
  };

  /**
   * Task (process) management.
   */
  class Task {
    public:
      using ControlBlock = TaskControlBlock;

      /**
       * Capability flags.
       */
      static constexpr UInt32 CapabilityIO = 1u << 0;

      /**
       * Initializes the task subsystem and creates the idle thread.
       */
      static void Initialize();

      /**
       * Creates a new kernel task with a single thread.
       * @param entryPoint
       *   Function pointer to the task's main thread entry point.
       * @param stackSize
       *   Size of the thread's kernel stack in bytes.
       * @return
       *   Pointer to the task control block, or `nullptr` on failure.
       */
      static ControlBlock* Create(
        void (*entryPoint)(),
        UInt32 stackSize = 4096
      );

      /**
       * Creates a new user task with a single thread.
       * @param entryPoint
       *   User-mode entry point address.
       * @param userStackTop
       *   Top of the user-mode stack.
       * @param pageDirectoryPhysical
       *   Physical address of the user address space page directory.
       * @return
       *   Pointer to the task control block, or `nullptr` on failure.
       */
      static ControlBlock* CreateUser(
        UInt32 entryPoint,
        UInt32 userStackTop,
        UInt32 pageDirectoryPhysical
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
       * Gets the currently executing task.
       * @return
       *   Pointer to the current task control block.
       */
      static ControlBlock* GetCurrent();

      /**
       * Gets the task id of the currently executing task (0 if none).
       * @return
       *   Task identifier.
       */
      static UInt32 GetCurrentId();

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
       * Records the coordinator task id for privileged operations.
       * @param taskId
       *   Task identifier.
       */
      static void SetCoordinatorId(UInt32 taskId);

      /**
       * Returns true if the current task is the coordinator.
       * @return
       *   True if the current task is the coordinator; false otherwise.
       */
      static bool IsCurrentTaskCoordinator();

      /**
       * Grants I/O access to the specified task.
       * @param taskId
       *   Task identifier.
       * @return
       *   True on success; false otherwise.
       */
      static bool GrantIOAccess(UInt32 taskId);

      /**
       * Returns true if the current task has I/O access.
       * @return
       *   True if the current task has I/O access; false otherwise.
       */
      static bool CurrentTaskHasIOAccess();

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
       * Releases a task and its resources (called by thread cleanup).
       * @param task
       *   Task control block to destroy.
       */
      static void Destroy(ControlBlock* task);

    private:
      /**
       * Creates a task control block without creating any threads.
       * @param pageDirectoryPhysical
       *   Physical address of the task page directory.
       * @return
       *   Pointer to the task control block, or `nullptr` on failure.
       */
      static ControlBlock* CreateInternal(UInt32 pageDirectoryPhysical);

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
       * Finds a task by id in the global task list.
       * @param id
       *   Task identifier to locate.
       * @return
       *   Pointer to the task control block, or `nullptr` if not found.
       */
      static ControlBlock* FindById(UInt32 id);

      /**
       * Task id of the coordinator task (for privileged operations).
       */
      inline static UInt32 _coordinatorTaskId = 0;

      /**
       * Head of the global task list.
       */
      inline static ControlBlock* _allTasksHead = nullptr;

      /**
       * Next task ID to assign.
       */
      inline static UInt32 _nextTaskId = 1;
  };
}
