//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/InterruptContext.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Interrupt context for IA32 architecture.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel {
  struct InterruptContext {
    // General-purpose registers (pusha order)
    UInt32 edi;
    UInt32 esi;
    UInt32 ebp;
    UInt32 esp;  // value before pusha
    UInt32 ebx;
    UInt32 edx;
    UInt32 ecx;
    UInt32 eax;

    // Software-pushed vector and hardware/synthetic error code
    UInt32 vector;
    UInt32 errorCode;

    // CPU-pushed state
    UInt32 eip;
    UInt32 cs;
    UInt32 eflags;
  };
}
