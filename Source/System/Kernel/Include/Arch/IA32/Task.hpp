/**
 * @file System/Kernel/Include/Arch/IA32/Task.hpp
 * @brief Backwards-compatible alias for IA32 thread types.
 */

#pragma once

#include "Arch/IA32/Thread.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using Task = Thread;
}
