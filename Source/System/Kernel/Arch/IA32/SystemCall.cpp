/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/SystemCall.cpp
 * IA32 system call setup.
 */

#include "Arch/IA32/IDT.hpp"
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/SystemCall.hpp"
#include "Handlers/SystemCallHandler.hpp"
#include "Prelude.hpp"
#include "Types.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using SystemCallHandler = Kernel::Handlers::SystemCallHandler;

  extern "C" void SYSCALL80();

  Interrupts::Context* SystemCall::OnSystemCall(Interrupts::Context& context) {
    return SystemCallHandler::Handle(context);
  }

  void SystemCall::Initialize() {
    IDT::SetGate(vector, SYSCALL80, 0xEE);
    Interrupts::RegisterHandler(vector, OnSystemCall);
  }
}
