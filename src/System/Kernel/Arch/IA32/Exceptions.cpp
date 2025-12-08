//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Exceptions.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Default IA32 exception handlers.
//------------------------------------------------------------------------------

#include <Interrupts.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/InterruptContext.hpp>
#include <Drivers/Console.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using Quantum::Kernel::Arch::IA32::CPU;
  using Quantum::Kernel::Drivers::Console;

  namespace {
    void DumpContext(const InterruptContext& ctx, uint32 faultAddr = 0) {
      Console::Write("EIP=");
      Console::WriteHex32(ctx.eip);
      Console::Write(" CS=");
      Console::WriteHex32(ctx.cs);
      Console::Write(" EFLAGS=");
      Console::WriteHex32(ctx.eflags);
      Console::WriteLine("");

      Console::Write("EAX=");
      Console::WriteHex32(ctx.eax);
      Console::Write(" EBX=");
      Console::WriteHex32(ctx.ebx);
      Console::Write(" ECX=");
      Console::WriteHex32(ctx.ecx);
      Console::Write(" EDX=");
      Console::WriteHex32(ctx.edx);
      Console::WriteLine("");

      Console::Write("ESI=");
      Console::WriteHex32(ctx.esi);
      Console::Write(" EDI=");
      Console::WriteHex32(ctx.edi);
      Console::Write(" EBP=");
      Console::WriteHex32(ctx.ebp);
      Console::Write(" ESP=");
      Console::WriteHex32(ctx.esp);
      Console::WriteLine("");

      Console::Write("Vector=");
      Console::WriteHex32(ctx.vector);
      Console::Write(" Error=");
      Console::WriteHex32(ctx.errorCode);
      
      if (faultAddr) {
        Console::Write(" CR2=");
        Console::WriteHex32(faultAddr);
      }

      Console::WriteLine("");
    }

    static void OnDivideByZero(InterruptContext&) {
      Console::WriteLine("Divide-by-zero fault (#DE)");
      CPU::HaltForever();
    }

    static void OnGeneralProtection(InterruptContext& ctx) {
      Console::WriteLine("General protection fault (#GP)");
      DumpContext(ctx);
      CPU::HaltForever();
    }

    static void OnPageFault(InterruptContext& ctx) {
      uint32 faultAddr;
      asm volatile("mov %%cr2, %0" : "=r"(faultAddr));
      Console::WriteLine("Page fault (#PF)");
      DumpContext(ctx, faultAddr);
      CPU::HaltForever();
    }
  }

  void InstallDefaultExceptionHandlers() {
    Interrupts::RegisterHandler(0, OnDivideByZero);
    Interrupts::RegisterHandler(13, OnGeneralProtection);
    Interrupts::RegisterHandler(14, OnPageFault);
  }
}
