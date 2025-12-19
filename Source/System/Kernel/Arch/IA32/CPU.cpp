/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/CPU.cpp
 * IA32 CPU control.
 */

#include <Logger.hpp>
#include <Prelude.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Types/Primitives.hpp>
#include <Types/Logging/LogLevel.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Kernel::Types::Logging::LogLevel;

  namespace {
    /**
     * Checks if the CPUID instruction is supported.
     * @return
     *   True if CPUID is supported, false otherwise.
     */
    static bool IsCPUIDSupported() {
      UInt32 flags1, flags2;

      // try to flip ID flag (bit 21) in EFLAGS
      asm volatile(
        "pushfl\n"                    // save EFLAGS
        "pop %%eax\n"                 // get EFLAGS into EAX
        "mov %%eax, %0\n"             // save original
        "xor $0x200000, %%eax\n"      // flip ID bit
        "push %%eax\n"                // set modified EFLAGS
        "popfl\n"
        "pushfl\n"                    // get EFLAGS back
        "pop %%eax\n"
        "mov %%eax, %1\n"             // save modified
        "push %0\n"                   // restore original EFLAGS
        "popfl\n"
        : "=r"(flags1), "=r"(flags2)
        :: "eax"
      );

      // if ID bit changed, CPUID is supported
      return (flags1 ^ flags2) & 0x200000;
    }

    /**
     * Executes the CPUID instruction with the given function ID.
     * @param function
     *   The CPUID function ID.
     * @param eax
     *   Reference to store EAX result.
     * @param ebx
     *   Reference to store EBX result.
     * @param ecx
     *   Reference to store ECX result.
     * @param edx
     *   Reference to store EDX result.
     */
    static void ExecuteCPUID(
      UInt32 function,
      UInt32& eax,
      UInt32& ebx, 
      UInt32& ecx,
      UInt32& edx
    ) {
      asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(function), "c"(0)
      );
    }
  }

  void CPU::Halt() {
    asm volatile("hlt");
  }

  void CPU::HaltForever() {
    Logger::Write(LogLevel::Info, "System halted");

    for (;;) {
      asm volatile("hlt");
    }
  }

  void CPU::DisableInterrupts() {
    asm volatile("cli" ::: "memory");
  }

  void CPU::EnableInterrupts() {
    asm volatile("sti" ::: "memory");
  }

  void CPU::LoadPageDirectory(UInt32 physicalAddress) {
    asm volatile("mov %0, %%cr3" :: "r"(physicalAddress) : "memory");
  }

  void CPU::EnablePaging() {
    UInt32 cr0;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 |= 0x80000000; // set PG bit

    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
  }

  void CPU::InvalidatePage(UInt32 address) {
    asm volatile("invlpg (%0)" :: "r"(address) : "memory");
  }

  CPU::Info CPU::GetInfo() {
    Info info = {};

    // check if CPUID is supported
    if (!IsCPUIDSupported()) {
      Logger::Write(LogLevel::Warning, "CPUID not supported on this CPU");

      // fill in minimal info for pre-486 CPUs
      for (int i = 0; i < 31; i++) {
        info.Vendor[i] = "Unknown"[i];

        if (info.Vendor[i] == '\0') break;
      }

      info.Vendor[31] = '\0';
      info.CoreCount = 1;

      return info;
    }

    UInt32 eax, ebx, ecx, edx;

    // function 0: get vendor string and max basic function
    ExecuteCPUID(0, eax, ebx, ecx, edx);
    info.MaxBasicFunction = eax;

    // Vendor string: EBX, EDX, ECX (in that order)
    *reinterpret_cast<UInt32*>(info.Vendor + 0) = ebx;
    *reinterpret_cast<UInt32*>(info.Vendor + 4) = edx;
    *reinterpret_cast<UInt32*>(info.Vendor + 8) = ecx;

    info.Vendor[12] = '\0';

    Logger::WriteFormatted(LogLevel::Info, "CPU Vendor: %s", info.Vendor);

    // function 1: get processor info and feature flags
    if (info.MaxBasicFunction >= 1) {
      ExecuteCPUID(1, eax, ebx, ecx, edx);

      // parse processor signature from EAX
      info.Stepping = eax & 0xF;
      info.ModelNumber = (eax >> 4) & 0xF;
      info.Family = (eax >> 8) & 0xF;
      info.ProcessorType = (eax >> 12) & 0x3;

      // extended model and family
      UInt32 extendedModel = (eax >> 16) & 0xF;
      UInt32 extendedFamily = (eax >> 20) & 0xFF;

      // adjust model and family if needed
      if (info.Family == 0xF) {
        info.Family += extendedFamily;
      }

      if (info.Family == 0x6 || info.Family == 0xF) {
        info.ModelNumber += (extendedModel << 4);
      }

      // parse feature flags from EDX (function 1)
      info.HasFPU = (edx & (1 << 0)) != 0;
      info.HasVME = (edx & (1 << 1)) != 0;
      info.HasDE = (edx & (1 << 2)) != 0;
      info.HasPSE = (edx & (1 << 3)) != 0;
      info.HasTSC = (edx & (1 << 4)) != 0;
      info.HasMSR = (edx & (1 << 5)) != 0;
      info.HasPAE = (edx & (1 << 6)) != 0;
      info.HasMCE = (edx & (1 << 7)) != 0;
      info.HasCX8 = (edx & (1 << 8)) != 0;
      info.HasAPIC = (edx & (1 << 9)) != 0;
      info.HasSEP = (edx & (1 << 11)) != 0;
      info.HasMTRR = (edx & (1 << 12)) != 0;
      info.HasPGE = (edx & (1 << 13)) != 0;
      info.HasMCA = (edx & (1 << 14)) != 0;
      info.HasCMOV = (edx & (1 << 15)) != 0;
      info.HasPAT = (edx & (1 << 16)) != 0;
      info.HasPSE36 = (edx & (1 << 17)) != 0;
      info.HasCLFSH = (edx & (1 << 19)) != 0;
      info.HasMMX = (edx & (1 << 23)) != 0;
      info.HasFXSR = (edx & (1 << 24)) != 0;
      info.HasSSE = (edx & (1 << 25)) != 0;
      info.HasSSE2 = (edx & (1 << 26)) != 0;
      info.HasHTT = (edx & (1 << 28)) != 0;

      // parse feature flags from ECX (function 1)
      info.HasSSE3 = (ecx & (1 << 0)) != 0;
      info.HasPCLMULQDQ = (ecx & (1 << 1)) != 0;
      info.HasSSSE3 = (ecx & (1 << 9)) != 0;
      info.HasFMA = (ecx & (1 << 12)) != 0;
      info.HasCX16 = (ecx & (1 << 13)) != 0;
      info.HasSSE41 = (ecx & (1 << 19)) != 0;
      info.HasSSE42 = (ecx & (1 << 20)) != 0;
      info.HasPOPCNT = (ecx & (1 << 23)) != 0;
      info.HasAES = (ecx & (1 << 25)) != 0;
      info.HasXSAVE = (ecx & (1 << 26)) != 0;
      info.HasAVX = (ecx & (1 << 28)) != 0;
      info.HasRDRAND = (ecx & (1 << 30)) != 0;

      // fill base fields
      info.HasHardwareFPU = info.HasFPU;
      info.HasSIMD = info.HasSSE || info.HasMMX;
      info.CoreCount = 1; // will be updated if we parse topology

      Logger::WriteFormatted(
        LogLevel::Info, 
        "CPU: Family=%u Model=%u Stepping=%u",
        info.Family,
        info.ModelNumber,
        info.Stepping
      );
    }

    // function 7: extended features
    if (info.MaxBasicFunction >= 7) {
      ExecuteCPUID(7, eax, ebx, ecx, edx);

      // parse extended feature flags from EBX
      info.HasFSGSBASE = (ebx & (1 << 0)) != 0;
      info.HasBMI1 = (ebx & (1 << 3)) != 0;
      info.HasAVX2 = (ebx & (1 << 5)) != 0;
      info.HasBMI2 = (ebx & (1 << 8)) != 0;
      info.HasRDSEED = (ebx & (1 << 18)) != 0;
      info.HasSMAP = (ebx & (1 << 20)) != 0;
      info.HasCLFLUSHOPT = (ebx & (1 << 23)) != 0;
    }

    // function 0x80000000: get max extended function
    ExecuteCPUID(0x80000000, eax, ebx, ecx, edx);

    info.MaxExtendedFunction = eax;

    // function 0x80000001: extended processor info and features
    if (info.MaxExtendedFunction >= 0x80000001) {
      ExecuteCPUID(0x80000001, eax, ebx, ecx, edx);

      // parse extended feature flags from EDX
      info.HasSYSCALL = (edx & (1 << 11)) != 0;
      info.HasNX = (edx & (1 << 20)) != 0;
      info.HasPage1GB = (edx & (1 << 26)) != 0;
      info.HasRDTSCP = (edx & (1 << 27)) != 0;
      info.HasLM = (edx & (1 << 29)) != 0;
      info.HasVirtualization = (ecx & (1 << 2)) != 0; // AMD SVM
    }

    // function 0x80000002-0x80000004: processor brand string
    if (info.MaxExtendedFunction >= 0x80000004) {
      char* modelPtr = info.Model;

      for (UInt32 func = 0x80000002; func <= 0x80000004; func++) {
        ExecuteCPUID(func, eax, ebx, ecx, edx);

        *reinterpret_cast<UInt32*>(modelPtr + 0) = eax;
        *reinterpret_cast<UInt32*>(modelPtr + 4) = ebx;
        *reinterpret_cast<UInt32*>(modelPtr + 8) = ecx;
        *reinterpret_cast<UInt32*>(modelPtr + 12) = edx;

        modelPtr += 16;
      }

      info.Model[47] = '\0';

      // trim leading spaces
      char* trimmed = info.Model;

      while (*trimmed == ' ') trimmed++;

      if (trimmed != info.Model) {
        UInt32 i = 0;

        while (trimmed[i] != '\0' && i < 63) {
          info.Model[i] = trimmed[i];
          i++;
        }

        info.Model[i] = '\0';
      }

      Logger::WriteFormatted(LogLevel::Info, "CPU Model: %s", info.Model);
    }
    
    Logger::WriteFormatted(
      LogLevel::Info, 
      "Features: FPU=%d SSE=%d SSE2=%d AVX=%d PAE=%d",
      info.HasFPU,
      info.HasSSE,
      info.HasSSE2,
      info.HasAVX,
      info.HasPAE
    );

    return info;
  }
}