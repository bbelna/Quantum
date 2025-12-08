//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Exceptions.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Default IA32 exception handlers.
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <Drivers/Console.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/InterruptContext.hpp>

using Quantum::Kernel::Drivers::Console;
using Quantum::Kernel::Arch::IA32::CPU;

namespace Quantum::Kernel::Arch::IA32 {
  namespace {
    static void OnDivideByZero(InterruptContext&) {
      Console::WriteLine("Divide-by-zero fault (#DE)");
      CPU::HaltForever();
    }

    static void OnGeneralProtection(InterruptContext& ctx) {
      Console::Write("General protection fault (#GP), error=");
      Console::Write("0x");

      // crude hex print for small value
      const char* hex = "0123456789ABCDEF";
      for (int shift = 28; shift >= 0; shift -= 4) {
        uint8 nibble = (ctx.errorCode >> shift) & 0xF;
        Console::WriteChar(hex[nibble]);
      }

      Console::WriteLine("");
      CPU::HaltForever();
    }

    static void OnPageFault(InterruptContext& ctx) {
      Console::Write("Page fault (#PF), error=");

      const char* hex = "0123456789ABCDEF";
      Console::Write("0x");

      for (int shift = 28; shift >= 0; shift -= 4) {
        uint8 nibble = (ctx.errorCode >> shift) & 0xF;
        Console::WriteChar(hex[nibble]);
      }

      Console::WriteLine("");

      CPU::HaltForever();
    }
  }

  void InstallDefaultExceptionHandlers() {
    Interrupts::RegisterHandler(0, OnDivideByZero);
    Interrupts::RegisterHandler(13, OnGeneralProtection);
    Interrupts::RegisterHandler(14, OnPageFault);
  }
}
