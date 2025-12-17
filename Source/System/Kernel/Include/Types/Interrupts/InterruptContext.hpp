/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Interrupts/InterruptContext.hpp
 * Interrupt context structure.
 */

#pragma once

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Types/IDT/InterruptContext.hpp>
#endif

namespace Quantum::System::Kernel::Types::Interrupts {
  #if defined(QUANTUM_ARCH_IA32)
    using InterruptContext
      = Quantum::System::Kernel::Arch::IA32::Types::IDT::InterruptContext;
  #else
    using InterruptContext = void;
  #endif
}
