/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Tasks/TaskState.hpp
 * Task state enumeration.
 */

#pragma once

namespace Quantum::System::Kernel::Types::Tasks {
  /**
   * Task state enumeration.
   */
  enum class TaskState : UInt8 {
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
}
