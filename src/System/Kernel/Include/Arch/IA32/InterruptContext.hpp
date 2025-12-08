//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/InterruptContext.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Interrupt context for IA32 architecture.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel {
  struct InterruptContext {
    uint8  vector;
    uint32 errorCode;
    // Later: saved registers, maybe flags, etc.
  };
}
