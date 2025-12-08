//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Interrupts.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 kernel interrupt subsystem implementation.
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>

namespace Quantum::Kernel {
  void Interrupts::Initialize() {
    Arch::IA32::InitializeIDT();
    Arch::IA32::InstallDefaultExceptionHandlers();
    Arch::IA32::CPU::EnableInterrupts();
  }

  void Interrupts::RegisterHandler(uint8 vector, InterruptHandler handler) {
    Arch::IA32::SetIDTHandler(vector, handler);
  }
}
