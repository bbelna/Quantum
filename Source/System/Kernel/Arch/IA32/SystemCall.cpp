/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/SystemCall.cpp
 * IA32 system call setup.
 */

#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/SystemCall.hpp>
#include <Interrupts.hpp>
#include <SystemCall.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  extern "C" void SYSCALL80();

  namespace {
    InterruptContext* OnSystemCall(InterruptContext& context) {
      return Kernel::SystemCall::Handle(context);
    }
  }

  void SystemCall::Initialize() {
    IDT::SetGate(Vector, SYSCALL80, 0xEE);
    Interrupts::RegisterHandler(Vector, OnSystemCall);
  }
}
