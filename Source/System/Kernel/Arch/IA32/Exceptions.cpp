//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Exceptions.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Default IA32 exception handlers.
//------------------------------------------------------------------------------

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/InterruptContext.hpp>
#include <Drivers/Console.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using CPU = Quantum::Kernel::Arch::IA32::CPU;
  using Kernel = Quantum::Kernel::Kernel;
  using LogLevel = Logger::Level;

  namespace {
    void DumpContext(
      const InterruptContext& context,
      UInt32 faultAddress = 0
    ) {
      Logger::WriteFormatted(
        LogLevel::Trace,
        "EIP=%p CS=%p EFLAGS=%p",
        context.eip,
        context.cs,
        context.eflags
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "EAX=%p EBX=%p ECX=%p EDX=%p",
        context.eax,
        context.ebx,
        context.ecx,
        context.edx
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "ESI=%p EDI=%p EBP=%p ESP=%p",
        context.esi,
        context.edi,
        context.ebp,
        context.esp
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "Vector=%p Error=%p%s",
        context.vector,
        context.errorCode
      );
      
      if (faultAddress) {
        Logger::WriteFormatted(
          LogLevel::Trace,
          "CR2=%p",
          faultAddress
        );
      }
    }

    static void OnDivideByZero(InterruptContext& context) {
      DumpContext(context);
      PANIC("Divide by zero fault");
    }

    static void OnGeneralProtection(InterruptContext& context) {
      DumpContext(context);
      PANIC("General protection fault");
    }

    static void OnPageFault(InterruptContext& context) {
      UInt32 faultAddress;

      asm volatile("mov %%cr2, %0" : "=r"(faultAddress));

      DumpContext(context, faultAddress);
      PANIC("Page fault");
    }
  }

  void InstallDefaultExceptionHandlers() {
    Interrupts::RegisterHandler(0, OnDivideByZero);
    Interrupts::RegisterHandler(13, OnGeneralProtection);
    Interrupts::RegisterHandler(14, OnPageFault);
  }
}
