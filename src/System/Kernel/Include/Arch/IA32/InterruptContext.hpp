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
    // General-purpose registers (pusha order)
    uint32 edi;
    uint32 esi;
    uint32 ebp;
    uint32 esp;  // value before pusha
    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;

    // Software-pushed vector and hardware/synthetic error code
    uint32 vector;
    uint32 errorCode;

    // CPU-pushed state
    uint32 eip;
    uint32 cs;
    uint32 eflags;
  };
}
