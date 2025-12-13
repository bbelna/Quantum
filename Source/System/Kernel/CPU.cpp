//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/CPU.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// CPU-related utilities and functions.
//------------------------------------------------------------------------------

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
