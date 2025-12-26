/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Interrupts.cpp
 * IA32 kernel interrupt subsystem implementation.
 */

#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Exceptions.hpp"
#include "Arch/IA32/IDT.hpp"
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/PS2Keyboard.hpp"
#include "Arch/IA32/SystemCall.hpp"
#include "Arch/IA32/Timer.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  void Interrupts::Initialize() {
    IDT::Initialize();
    Exceptions::InstallDefaultHandlers();

    SystemCall::Initialize();

    Timer::Initialize();
    PS2Keyboard::Initialize();

    CPU::EnableInterrupts();
  }

  void Interrupts::RegisterHandler(UInt8 vector, Interrupts::Handler handler) {
    IDT::SetHandler(vector, handler);
  }
}
