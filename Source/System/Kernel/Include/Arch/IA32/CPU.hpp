/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/CPU.hpp
 * IA32 CPU control.
 */

#pragma once

#include <CPU.hpp>
#include <Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 CPU control.
   */
  class CPU {
    public:
      /**
       * IA32 CPU information.
       */
      class Info : public Kernel::CPU::Info {
        public:
          /**
           * Highest basic CPUID function supported (range 0x0-0x7FFFFFFF).
           */
          UInt32 maxBasicFunction;

          /**
           * Highest extended CPUID function supported (range 0x80000000+).
           */
          UInt32 maxExtendedFunction;

          /**
           * Processor stepping identifier within the model.
           */
          UInt32 stepping;

          /**
           * Processor model number.
           */
          UInt32 modelNumber;

          /**
           * Processor family number.
           */
          UInt32 family;

          /**
           * Processor type (0=OEM, 1=OverDrive, 2=Dual processor).
           */
          UInt32 processorType;

          /**
           * Indicates if the CPU has an x87 floating-point unit on chip.
           */
          bool hasFPU;

          /**
           * Indicates if the CPU supports virtual 8086 mode enhancements.
           */
          bool hasVME;

          /**
           * Indicates if the CPU supports debugging extensions (I/O breakpoints).
           */
          bool hasDE;

          /**
           * Indicates if the CPU supports page size extension (4MB pages).
           */
          bool hasPSE;

          /**
           * Indicates if the CPU has a time stamp counter (RDTSC instruction).
           */
          bool hasTSC;

          /**
           * Indicates if the CPU supports model-specific registers
           * (RDMSR/WRMSR instructions).
           */
          bool hasMSR;

          /**
           * Indicates if the CPU supports physical address extension
           * (36-bit addressing).
           */
          bool hasPAE;

          /**
           * Indicates if the CPU supports machine check exception (MCE).
           */
          bool hasMCE;

          /**
           * Indicates if the CPU supports the CMPXCHG8B instruction.
           */
          bool hasCX8;

          /**
           * Indicates if the CPU has an on-chip Advanced Programmable Interrupt
           * Controller.
           */
          bool hasAPIC;

          /**
           * Indicates if the CPU supports SYSENTER and SYSEXIT fast system call
           * instructions.
           */
          bool hasSEP;

          /**
           * Indicates if the CPU supports memory type range registers for cache
           * control.
           */
          bool hasMTRR;

          /**
           * Indicates if the CPU supports page global enable bit in CR4.
           */
          bool hasPGE;

          /**
           * Indicates if the CPU supports machine check architecture.
           */
          bool hasMCA;

          /**
           * Indicates if the CPU supports conditional move instructions (CMOV).
           */
          bool hasCMOV;

          /**
           * Indicates if the CPU supports page attribute table for memory types.
           */
          bool hasPAT;

          /**
           * Indicates if the CPU supports 36-bit page size extension.
           */
          bool hasPSE36;

          /**
           * Indicates if the CPU supports the CLFLUSH cache line flush
           * instruction.
           */
          bool hasCLFSH;

          /**
           * Indicates if the CPU supports MMX technology instructions.
           */
          bool hasMMX;

          /**
           * Indicates if the CPU supports FXSAVE and FXRSTOR instructions for FPU
           * state.
           */
          bool hasFXSR;

          /**
           * Indicates if the CPU supports Streaming SIMD Extensions (SSE).
           */
          bool hasSSE;

          /**
           * Indicates if the CPU supports Streaming SIMD Extensions 2 (SSE2).
           */
          bool hasSSE2;

          /**
           * Indicates if the CPU supports hyper-threading technology.
           */
          bool hasHTT;

          /**
           * Indicates if the CPU supports Streaming SIMD Extensions 3 (SSE3).
           */
          bool hasSSE3;

          /**
           * Indicates if the CPU supports the PCLMULQDQ carry-less multiplication
           * instruction.
           */
          bool hasPCLMULQDQ;

          /**
           * Indicates if the CPU supports Supplemental Streaming SIMD Extensions
           * 3 (SSSE3).
           */
          bool hasSSSE3;

          /**
           * Indicates if the CPU supports fused multiply-add instructions (FMA3).
           */
          bool hasFMA;

          /**
           * Indicates if the CPU supports the CMPXCHG16B instruction for 16-byte
           * compare-and-swap.
           */
          bool hasCX16;

          /**
           * Indicates if the CPU supports Streaming SIMD Extensions 4.1 (SSE4.1).
           */
          bool hasSSE41;

          /**
           * Indicates if the CPU supports Streaming SIMD Extensions 4.2 (SSE4.2).
           */
          bool hasSSE42;

          /**
           * Indicates if the CPU supports the POPCNT population count
           * instruction.
           */
          bool hasPOPCNT;

          /**
           * Indicates if the CPU supports AES instruction set for encryption.
           */
          bool hasAES;

          /**
           * Indicates if the CPU supports XSAVE and XRSTOR instructions for
           * extended state management.
           */
          bool hasXSAVE;

          /**
           * Indicates if the CPU supports Advanced Vector Extensions (AVX).
           */
          bool hasAVX;

          /**
           * Indicates if the CPU supports the RDRAND hardware random number
           * generator instruction.
           */
          bool hasRDRAND;

          /**
           * Indicates if the CPU supports RDFSBASE and WRFSBASE instructions for
           * FS/GS base access.
           */
          bool hasFSGSBASE;

          /**
           * Indicates if the CPU supports Bit Manipulation Instruction Set 1
           * (BMI1).
           */
          bool hasBMI1;

          /**
           * Indicates if the CPU supports Advanced Vector Extensions 2 (AVX2).
           */
          bool hasAVX2;

          /**
           * Indicates if the CPU supports Bit Manipulation Instruction Set 2
           * (BMI2).
           */
          bool hasBMI2;

          /**
           * Indicates if the CPU supports the RDSEED hardware random number seed
           * instruction.
           */
          bool hasRDSEED;

          /**
           * Indicates if the CPU supports Supervisor Mode Access Prevention.
           */
          bool hasSMAP;

          /**
           * Indicates if the CPU supports the optimized CLFLUSHOPT cache line
           * flush instruction.
           */
          bool hasCLFLUSHOPT;

          /**
           * Indicates if the CPU supports SYSCALL and SYSRET fast system call
           * instructions.
           */
          bool hasSYSCALL;

          /**
           * Indicates if the CPU supports the NX (No-Execute) bit for memory
           * protection.
           */
          bool hasNX;

          /**
           * Indicates if the CPU supports 1GB memory pages.
           */
          bool hasPage1GB;

          /**
           * Indicates if the CPU supports the RDTSCP instruction for reading
           * time-stamp counter.
           */
          bool hasRDTSCP;

          /**
           * Indicates if the CPU supports Long Mode (64-bit x86-64 architecture).
           */
          bool hasLM;
      };

      /**
       * Halts the CPU until the next interrupt.
       */
      static void Halt();

      /**
       * Halts the CPU forever.
       */
      [[noreturn]] static void HaltForever();

      /**
       * Disable interrupts.
       */
      static void DisableInterrupts();

      /**
       * Enable interrupts.
       */
      static void EnableInterrupts();

      /**
       * Loads the physical address of the page directory into CR3.
       * @param physicalAddress
       *   The physical address of the page directory.
       */
      static void LoadPageDirectory(UInt32 physicalAddress);

      /**
       * Enables paging by setting the PG bit in CR0.
       */
      static void EnablePaging();

      /**
       * Invalidates a single page from the TLB.
       * @param address
       *   The virtual address of the page to invalidate.
       */
      static void InvalidatePage(UInt32 address);

      /**
       * Executes a pause instruction to improve spin-wait loops.
       */
      static void Pause();

      /**
       * Retrieves IA32 CPU information.
       * @return `Info` structure with CPU details.
       */
      static Info GetInfo();

    private:
      /**
       * Checks if the CPUID instruction is supported.
       * @return
       *   True if CPUID is supported, false otherwise.
       */
      static bool IsCPUIDSupported();

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
      );
  };
}
