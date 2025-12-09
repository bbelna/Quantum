//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Kernel.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// The core kernel implementation for Quantum.
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Memory.hpp>
#include <Drivers/Console.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/CPU.hpp>

  using ArchCPU = Quantum::Kernel::Arch::IA32::CPU;
#else
  #error "No architecture selected for kernel"
#endif

namespace Quantum::Kernel {
  using namespace Drivers;

  void Kernel::Initialize(uint32 bootInfoPhysicalAddress) {
    Console::Initialize();
    Console::WriteLine("Quantum");

    Memory::Initialize(bootInfoPhysicalAddress);
    Interrupts::Initialize();
  }

  void Kernel::Panic(const char* message) {
    Console::Write("KERNEL PANIC: ");
    Console::WriteLine(message);
    ArchCPU::HaltForever();
  }
}
