//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/InterruptContext.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Interrupt context for IA32 architecture.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel {
  /**
   * Register snapshot captured on interrupt entry for IA32.
   */
  struct InterruptContext {
    /**
     * General-purpose register EDI (pusha order).
     */
    UInt32 edi;

    /**
     * General-purpose register ESI (pusha order).
     */
    UInt32 esi;

    /**
     * Base pointer captured during pusha.
     */
    UInt32 ebp;

    /**
     * Value before pusha for ESP.
     */
    UInt32 esp;

    /**
     * General-purpose register EBX (pusha order).
     */
    UInt32 ebx;

    /**
     * General-purpose register EDX (pusha order).
     */
    UInt32 edx;

    /**
     * General-purpose register ECX (pusha order).
     */
    UInt32 ecx;

    /**
     * General-purpose register EAX (pusha order).
     */
    UInt32 eax;

    /**
     * Software-pushed vector.
     */
    UInt32 vector;

    /**
     * Software-pushed hardware/synthetic error code.
     */
    UInt32 errorCode;

    /**
     * Instruction pointer at the time of the interrupt.
     */
    UInt32 eip;

    /**
     * Code segment selector at the time of the interrupt.
     */
    UInt32 cs;

    /**
     * CPU flags at the time of the interrupt.
     */
    UInt32 eflags;
  };
}
