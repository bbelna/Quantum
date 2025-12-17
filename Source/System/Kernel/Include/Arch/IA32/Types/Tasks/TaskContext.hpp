/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/Tasks/TaskContext.hpp
 * IA32 task context structure.
 */

#pragma once

#include <Arch/IA32/Types/IDT/InterruptContext.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::Tasks {
  /**
   * Task context structure for IA32 architecture.
   */
  using TaskContext = Types::IDT::InterruptContext;
}
