/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Interrupts.cpp
 * Architecture-agnostic kernel interrupt subsystem.
 */

#include <Interrupts.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/Interrupts.hpp>
#endif

namespace Quantum::System::Kernel {
  #if defined(QUANTUM_ARCH_IA32)
  using ArchInterrupts = Arch::IA32::Interrupts;
  #endif

  void Interrupts::Initialize() {
    ArchInterrupts::Initialize();
  }

  void Interrupts::RegisterHandler(UInt8 vector, Interrupts::Handler handler) {
    ArchInterrupts::RegisterHandler(vector, handler);
  }
}
