//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Interrupts.cpp
// (c) 2025 Brandon Belna - MIT LIcense
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
  using namespace Quantum::Kernel::Arch;

  void Interrupts::Initialize() {
    IA32::InitializeIDT();
    IA32::InstallDefaultExceptionHandlers();

    IA32::Drivers::Timer::Initialize();
    IA32::Drivers::PS2Keyboard::Initialize();

    IA32::CPU::EnableInterrupts();
  }

  void Interrupts::RegisterHandler(UInt8 vector, InterruptHandler handler) {
    IA32::SetIDTHandler(vector, handler);
  }
}
