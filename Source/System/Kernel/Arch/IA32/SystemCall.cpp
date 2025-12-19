/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/SystemCall.cpp
 * IA32 system call setup.
 */

#include <Arch/IA32/IDT.hpp>
#include <Arch/IA32/SystemCall.hpp>
#include <Arch/IA32/Interrupts.hpp>
#include <Kernel.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  extern "C" void SYSCALL80();

  namespace {
    Interrupts::Context* OnSystemCall(Interrupts::Context& context) {
      return Kernel::HandleSystemCall(context);
    }
  }

  void SystemCall::Initialize() {
    IDT::SetGate(vector, SYSCALL80, 0xEE);
    Interrupts::RegisterHandler(vector, OnSystemCall);
  }
}
