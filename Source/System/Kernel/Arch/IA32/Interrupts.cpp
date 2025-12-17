/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Interrupts.cpp
 * IA32 kernel interrupt subsystem implementation.
 */

#include <Interrupts.hpp>
#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/PS2Keyboard.hpp>
#include <Arch/IA32/Timer.hpp>

namespace Quantum::System::Kernel {
  using namespace Quantum::System::Kernel::Arch;

  void Interrupts::Initialize() {
    IA32::IDT::Initialize();
    IA32::Exceptions::InstallDefaultHandlers();

    IA32::Timer::Initialize();
    IA32::PS2Keyboard::Initialize();

    IA32::CPU::EnableInterrupts();
  }

  void Interrupts::RegisterHandler(UInt8 vector, InterruptHandler handler) {
    IA32::IDT::SetHandler(vector, handler);
  }
}
