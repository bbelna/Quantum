/**
 * @file System/Kernel/Arch/IA32/CPU.cpp
 * @brief IA32 CPU handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/CPU.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Kernel::Logger::Level;

  bool CPU::IsCPUIDSupported() {
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

  void CPU::ExecuteCPUID(
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

  void CPU::Pause() {
    asm volatile("pause");
  }

  CPU::Info CPU::GetInfo() {
    Info info = {};

    // check if CPUID is supported
    if (!IsCPUIDSupported()) {
      Logger::Write(LogLevel::Warning, "CPUID not supported on this CPU");

      // fill in minimal info for pre-486 CPUs
      for (int i = 0; i < 31; i++) {
        info.vendor[i] = "Unknown"[i];

        if (info.vendor[i] == '\0') break;
      }

      info.vendor[31] = '\0';
      info.coreCount = 1;

      return info;
    }

    UInt32 eax, ebx, ecx, edx;

    // function 0: get vendor string and max basic function
    ExecuteCPUID(0, eax, ebx, ecx, edx);
    info.maxBasicFunction = eax;

    // Vendor string: EBX, EDX, ECX (in that order)
    *reinterpret_cast<UInt32*>(info.vendor + 0) = ebx;
    *reinterpret_cast<UInt32*>(info.vendor + 4) = edx;
    *reinterpret_cast<UInt32*>(info.vendor + 8) = ecx;

    info.vendor[12] = '\0';

    Logger::WriteFormatted(LogLevel::Info, "CPU Vendor: %s", info.vendor);

    // function 1: get processor info and feature flags
    if (info.maxBasicFunction >= 1) {
      ExecuteCPUID(1, eax, ebx, ecx, edx);

      // parse processor signature from EAX
      info.stepping = eax & 0xF;
      info.modelNumber = (eax >> 4) & 0xF;
      info.family = (eax >> 8) & 0xF;
      info.processorType = (eax >> 12) & 0x3;

      // extended model and family
      UInt32 extendedModel = (eax >> 16) & 0xF;
      UInt32 extendedFamily = (eax >> 20) & 0xFF;

      // adjust model and family if needed
      if (info.family == 0xF) {
        info.family += extendedFamily;
      }

      if (info.family == 0x6 || info.family == 0xF) {
        info.modelNumber += (extendedModel << 4);
      }

      // parse feature flags from EDX (function 1)
      info.hasFPU = (edx & (1 << 0)) != 0;
      info.hasVME = (edx & (1 << 1)) != 0;
      info.hasDE = (edx & (1 << 2)) != 0;
      info.hasPSE = (edx & (1 << 3)) != 0;
      info.hasTSC = (edx & (1 << 4)) != 0;
      info.hasMSR = (edx & (1 << 5)) != 0;
      info.hasPAE = (edx & (1 << 6)) != 0;
      info.hasMCE = (edx & (1 << 7)) != 0;
      info.hasCX8 = (edx & (1 << 8)) != 0;
      info.hasAPIC = (edx & (1 << 9)) != 0;
      info.hasSEP = (edx & (1 << 11)) != 0;
      info.hasMTRR = (edx & (1 << 12)) != 0;
      info.hasPGE = (edx & (1 << 13)) != 0;
      info.hasMCA = (edx & (1 << 14)) != 0;
      info.hasCMOV = (edx & (1 << 15)) != 0;
      info.hasPAT = (edx & (1 << 16)) != 0;
      info.hasPSE36 = (edx & (1 << 17)) != 0;
      info.hasCLFSH = (edx & (1 << 19)) != 0;
      info.hasMMX = (edx & (1 << 23)) != 0;
      info.hasFXSR = (edx & (1 << 24)) != 0;
      info.hasSSE = (edx & (1 << 25)) != 0;
      info.hasSSE2 = (edx & (1 << 26)) != 0;
      info.hasHTT = (edx & (1 << 28)) != 0;

      // parse feature flags from ECX (function 1)
      info.hasSSE3 = (ecx & (1 << 0)) != 0;
      info.hasPCLMULQDQ = (ecx & (1 << 1)) != 0;
      info.hasSSSE3 = (ecx & (1 << 9)) != 0;
      info.hasFMA = (ecx & (1 << 12)) != 0;
      info.hasCX16 = (ecx & (1 << 13)) != 0;
      info.hasSSE41 = (ecx & (1 << 19)) != 0;
      info.hasSSE42 = (ecx & (1 << 20)) != 0;
      info.hasPOPCNT = (ecx & (1 << 23)) != 0;
      info.hasAES = (ecx & (1 << 25)) != 0;
      info.hasXSAVE = (ecx & (1 << 26)) != 0;
      info.hasAVX = (ecx & (1 << 28)) != 0;
      info.hasRDRAND = (ecx & (1 << 30)) != 0;

      // fill base fields
      info.hasHardwareFPU = info.hasFPU;
      info.hasSIMD = info.hasSSE || info.hasMMX;
      info.coreCount = 1; // will be updated if we parse topology

      Logger::WriteFormatted(
        LogLevel::Info, 
        "CPU: Family=%u Model=%u Stepping=%u",
        info.family,
        info.modelNumber,
        info.stepping
      );
    }

    // function 7: extended features
    if (info.maxBasicFunction >= 7) {
      ExecuteCPUID(7, eax, ebx, ecx, edx);

      // parse extended feature flags from EBX
      info.hasFSGSBASE = (ebx & (1 << 0)) != 0;
      info.hasBMI1 = (ebx & (1 << 3)) != 0;
      info.hasAVX2 = (ebx & (1 << 5)) != 0;
      info.hasBMI2 = (ebx & (1 << 8)) != 0;
      info.hasRDSEED = (ebx & (1 << 18)) != 0;
      info.hasSMAP = (ebx & (1 << 20)) != 0;
      info.hasCLFLUSHOPT = (ebx & (1 << 23)) != 0;
    }

    // function 0x80000000: get max extended function
    ExecuteCPUID(0x80000000, eax, ebx, ecx, edx);

    info.maxExtendedFunction = eax;

    // function 0x80000001: extended processor info and features
    if (info.maxExtendedFunction >= 0x80000001) {
      ExecuteCPUID(0x80000001, eax, ebx, ecx, edx);

      // parse extended feature flags from EDX
      info.hasSYSCALL = (edx & (1 << 11)) != 0;
      info.hasNX = (edx & (1 << 20)) != 0;
      info.hasPage1GB = (edx & (1 << 26)) != 0;
      info.hasRDTSCP = (edx & (1 << 27)) != 0;
      info.hasLM = (edx & (1 << 29)) != 0;
      info.hasVirtualization = (ecx & (1 << 2)) != 0; // AMD SVM
    }

    // function 0x80000002-0x80000004: processor brand string
    if (info.maxExtendedFunction >= 0x80000004) {
      char* modelPtr = info.model;

      for (UInt32 func = 0x80000002; func <= 0x80000004; func++) {
        ExecuteCPUID(func, eax, ebx, ecx, edx);

        *reinterpret_cast<UInt32*>(modelPtr + 0) = eax;
        *reinterpret_cast<UInt32*>(modelPtr + 4) = ebx;
        *reinterpret_cast<UInt32*>(modelPtr + 8) = ecx;
        *reinterpret_cast<UInt32*>(modelPtr + 12) = edx;

        modelPtr += 16;
      }

      info.model[47] = '\0';

      // trim leading spaces
      char* trimmed = info.model;

      while (*trimmed == ' ') trimmed++;

      if (trimmed != info.model) {
        UInt32 i = 0;

        while (trimmed[i] != '\0' && i < 63) {
          info.model[i] = trimmed[i];
          i++;
        }

        info.model[i] = '\0';
      }

      Logger::WriteFormatted(LogLevel::Info, "CPU Model: %s", info.model);
    }
    
    Logger::WriteFormatted(
      LogLevel::Info, 
      "Features: FPU=%d SSE=%d SSE2=%d AVX=%d PAE=%d",
      info.hasFPU,
      info.hasSSE,
      info.hasSSE2,
      info.hasAVX,
      info.hasPAE
    );

    return info;
  }
}
