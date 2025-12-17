/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/Tasks/TaskControlBlock.hpp
 * Task control block for IA32 architecture.
 */

#pragma once

#include <Prelude.hpp>
#include <Arch/IA32/Types/Tasks/TaskContext.hpp>
#include <Types/Primitives.hpp>
#include <Types/Tasks/TaskState.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::Tasks {
  using Kernel::Types::Tasks::TaskState;

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
     * Pointer to the saved interrupt context for the task.
     */
    TaskContext* Context;

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
}