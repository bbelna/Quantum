/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Exceptions.cpp
 * IA32 exception handler registration.
 */

#include <Kernel.hpp>
#include <Logger.hpp>
#include <Prelude.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/Interrupts.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Kernel::Types::Logging::LogLevel;

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
        context.EIP,
        context.CS,
        context.EFlags
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "EAX=%p EBX=%p ECX=%p EDX=%p",
        context.EAX,
        context.EBX,
        context.ECX,
        context.EDX
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "ESI=%p EDI=%p EBP=%p ESP=%p",
        context.ESI,
        context.EDI,
        context.EBP,
        context.ESP
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "Vector=%p Error=%p",
        context.Vector,
        context.ErrorCode
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
        context.ErrorCode
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
