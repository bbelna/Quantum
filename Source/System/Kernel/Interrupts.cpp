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
  #include <Arch/IA32/Prelude.hpp>

  using ArchInterrupts = KernelIA32::Interrupts;
#endif

namespace Quantum::System::Kernel {
  void Interrupts::Initialize() {
    ArchInterrupts::Initialize();
  }

  void Interrupts::RegisterHandler(UInt8 vector, InterruptHandler handler) {
    ArchInterrupts::RegisterHandler(vector, handler);
  }
}
