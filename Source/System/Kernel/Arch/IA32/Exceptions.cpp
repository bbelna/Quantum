/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Exceptions.cpp
 * IA32 exception handler registration.
 */

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Exceptions.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Arch/IA32/Types/IDT/InterruptContext.hpp>
#include <Interrupts.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Types/Logging/Level.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  namespace QK = Quantum::System::Kernel;

  using CPU = CPU;
  using InterruptContext = Types::IDT::InterruptContext;
  using LogLevel = QK::Types::Logging::Level;

  namespace {
    /**
     * Dumps the interrupt context to the kernel log.
     * @param context
     *   The interrupt context.
     * @param faultAddress
     *   Optional, defaults to 0. The faulting address (for page faults).
     */
    void DumpContext(
      const InterruptContext& context,
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

    static void OnDivideByZero(InterruptContext& context) {
      DumpContext(context);
      PANIC("Divide by zero fault");
    }

    static void OnGeneralProtection(InterruptContext& context) {
      DumpContext(context);
      PANIC("General protection fault");
    }

    void DumpPageFaultDetails(UInt32 faultAddress, UInt32 errorCode) {
      const char* accessType = (errorCode & 0x2) ? "write" : "read";
      const char* mode = (errorCode & 0x4) ? "user" : "kernel";
      bool presentViolation = (errorCode & 0x1) != 0;
      bool reservedBit = (errorCode & 0x8) != 0;
      bool instructionFetch = (errorCode & 0x10) != 0;

      UInt32 pde = Memory::GetPageDirectoryEntry(faultAddress);
      UInt32 pte = Memory::GetPageTableEntry(faultAddress);

      Logger::WriteFormatted(
        LogLevel::Trace,
        "Page fault at %p (%s %s) err=%p present=%s reserved=%s instr=%s",
        faultAddress,
        accessType,
        mode,
        errorCode,
        presentViolation ? "yes" : "no",
        reservedBit ? "yes" : "no",
        instructionFetch ? "yes" : "no"
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "PDE=%p PTE=%p",
        pde,
        pte
      );
    }

    static void OnPageFault(InterruptContext& context) {
      UInt32 faultAddress;

      asm volatile("mov %%cr2, %0" : "=r"(faultAddress));

      DumpContext(context, faultAddress);
      DumpPageFaultDetails(faultAddress, context.ErrorCode);

      PANIC("Page fault");
    }
  }

  void Exceptions::InstallDefaultHandlers() {
    Interrupts::RegisterHandler(0, OnDivideByZero);
    Interrupts::RegisterHandler(13, OnGeneralProtection);
    Interrupts::RegisterHandler(14, OnPageFault);
  }
}
