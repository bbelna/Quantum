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
#include <Arch/IA32/Drivers/Timer.hpp>
#include <Arch/IA32/Drivers/PS2Keyboard.hpp>

namespace Quantum::Kernel {
  using Quantum::Kernel::Arch::IA32::CPU;
  using Quantum::Kernel::Arch::IA32::InitializeIDT;
  using Quantum::Kernel::Arch::IA32::InstallDefaultExceptionHandlers;
  using Quantum::Kernel::Arch::IA32::SetIDTHandler;
  using Quantum::Kernel::Arch::IA32::Drivers::Timer;
  using Quantum::Kernel::Arch::IA32::Drivers::PS2Keyboard;

  void Interrupts::Initialize() {
    InitializeIDT();
    InstallDefaultExceptionHandlers();

    Timer::Initialize();
    PS2Keyboard::Initialize();

    CPU::EnableInterrupts();
  }

  void Interrupts::RegisterHandler(uint8 vector, InterruptHandler handler) {
    SetIDTHandler(vector, handler);
  }
}
