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

  namespace Arch = Quantum::Kernel::Arch::IA32;
  using ArchCPU = Arch::CPU;
#else
  #error "No architecture selected for CPU"
#endif

namespace Quantum::Kernel {
  /**
   * Halts the CPU forever.
   */
  void CPU::HaltForever() {
    ArchCPU::HaltForever();
  }
}
