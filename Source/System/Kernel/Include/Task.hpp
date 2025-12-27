/**
 * @file System/Kernel/Include/Task.hpp
 * @brief Architecture-agnostic task management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#include "Arch/Task.hpp"
#include "Interrupts.hpp"

namespace Quantum::System::Kernel {
  /**
   * Task management and scheduling.
   */
  class Task {
    public:
      using ControlBlock = Arch::Task::ControlBlock;

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
       * Creates a new user task in the specified address space.
       * @param entryPoint
       *   User-mode entry point address.
       * @param userStackTop
       *   Top of the user-mode stack.
       * @param pageDirectoryPhysical
       *   Physical address of the user address space page directory.
       * @return
       *   Pointer to the task control block, or nullptr on failure.
       */
      static ControlBlock* CreateUser(
        UInt32 entryPoint,
        UInt32 userStackTop,
        UInt32 pageDirectoryPhysical
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

    private:
      /**
       * Task id of the coordinator task (for privileged operations).
       */
      inline static UInt32 _coordinatorTaskId = 0;
  };
}
