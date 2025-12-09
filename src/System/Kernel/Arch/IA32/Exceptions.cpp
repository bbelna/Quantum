//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Exceptions.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Default IA32 exception handlers.
//------------------------------------------------------------------------------

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/InterruptContext.hpp>
#include <Drivers/Console.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using Quantum::Kernel::Arch::IA32::CPU;
  using Quantum::Kernel::Drivers::Console;
  using Quantum::Kernel::Kernel;

  namespace {
    void DumpContext(
      const InterruptContext& context,
      uint32 faultAddress = 0
    ) {
      Console::Write("EIP=");
      Console::WriteHex32(context.eip);
      Console::Write(" CS=");
      Console::WriteHex32(context.cs);
      Console::Write(" EFLAGS=");
      Console::WriteHex32(context.eflags);
      Console::WriteLine("");

      Console::Write("EAX=");
      Console::WriteHex32(context.eax);
      Console::Write(" EBX=");
      Console::WriteHex32(context.ebx);
      Console::Write(" ECX=");
      Console::WriteHex32(context.ecx);
      Console::Write(" EDX=");
      Console::WriteHex32(context.edx);
      Console::WriteLine("");

      Console::Write("ESI=");
      Console::WriteHex32(context.esi);
      Console::Write(" EDI=");
      Console::WriteHex32(context.edi);
      Console::Write(" EBP=");
      Console::WriteHex32(context.ebp);
      Console::Write(" ESP=");
      Console::WriteHex32(context.esp);
      Console::WriteLine("");

      Console::Write("Vector=");
      Console::WriteHex32(context.vector);
      Console::Write(" Error=");
      Console::WriteHex32(context.errorCode);
      
      if (faultAddress) {
        Console::Write(" CR2=");
        Console::WriteHex32(faultAddress);
      }

      Console::WriteLine("");
    }

    static void OnDivideByZero(InterruptContext& context) {
      DumpContext(context);
      Kernel::Panic("Divide by zero fault");
    }

    static void OnGeneralProtection(InterruptContext& context) {
      DumpContext(context);
      Kernel::Panic("General protection fault");
    }

    static void OnPageFault(InterruptContext& context) {
      uint32 faultAddress;

      asm volatile("mov %%cr2, %0" : "=r"(faultAddress));

      DumpContext(context, faultAddress);
      Kernel::Panic("Page fault");
    }
  }

  void InstallDefaultExceptionHandlers() {
    Interrupts::RegisterHandler(0, OnDivideByZero);
    Interrupts::RegisterHandler(13, OnGeneralProtection);
    Interrupts::RegisterHandler(14, OnPageFault);
  }
}
