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
  /**
   * Halts the CPU forever.
   */
  void CPU::HaltForever() {
    ArchCPU::HaltForever();
  }
}
