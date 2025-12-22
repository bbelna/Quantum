/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/CPU.cpp
 * Architecture-agnostic CPU driver.
 */

#include <CPU.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/CPU.hpp>

namespace Arch = Quantum::System::Kernel::Arch::IA32;
using ArchCPU = Arch::CPU;
#endif

namespace Quantum::System::Kernel {
  void CPU::HaltForever() {
    ArchCPU::HaltForever();
  }

  void CPU::Pause() {
    ArchCPU::Pause();
  }

  void CPU::DisableInterrupts() {
    ArchCPU::DisableInterrupts();
  }

  void CPU::EnableInterrupts() {
    ArchCPU::EnableInterrupts();
  }
}
