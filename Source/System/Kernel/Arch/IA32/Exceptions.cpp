/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Exceptions.cpp
 * IA32 exception handler registration.
 */

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/Interrupts.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Prelude.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Logger::Level;

  namespace {
    /**
     * Dumps the interrupt context to the kernel log.
     * @param context
     *   The interrupt context.
     * @param faultAddress
     *   Optional, defaults to 0. The faulting address (for page faults).
     */
    void DumpContext(
      const Interrupts::Context& context,
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
        "Vector=%p Error=%p",
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

    static Interrupts::Context* OnDivideByZero(Interrupts::Context& context) {
      DumpContext(context);
      PANIC("Divide by zero fault");

      return &context;
    }

    static Interrupts::Context* OnGeneralProtection(Interrupts::Context& context) {
      DumpContext(context);
      PANIC("General protection fault");

      return &context;
    }

    static Interrupts::Context* OnPageFault(Interrupts::Context& context) {
      UInt32 faultAddress;

      asm volatile("mov %%cr2, %0" : "=r"(faultAddress));

      DumpContext(context, faultAddress);

      bool handled = Memory::HandlePageFault(
        context,
        faultAddress,
        context.errorCode
      );

      if (!handled) {
        PANIC("Page fault");
      }

      return &context;
    }
  }

  void Exceptions::InstallDefaultHandlers() {
    Interrupts::RegisterHandler(0, OnDivideByZero);
    Interrupts::RegisterHandler(13, OnGeneralProtection);
    Interrupts::RegisterHandler(14, OnPageFault);
  }
}
