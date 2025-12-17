/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/CPU/IA32CPUInfo.hpp
 * IA32-specific CPU information.
 */

#pragma once

#include <Prelude.hpp>
#include <Types/Primitives.hpp>
#include <Types/CPU/CPUInfo.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::CPU {
  using Kernel::Types::CPU::CPUInfo;

  /**
   * IA32-specific CPU information.
   */
  struct IA32CPUInfo : public CPUInfo {
    /**
     * Highest basic CPUID function supported (range 0x0-0x7FFFFFFF).
     */
    UInt32 MaxBasicFunction;
    
    /**
     * Highest extended CPUID function supported (range 0x80000000+).
     */
    UInt32 MaxExtendedFunction;
    
    /**
     * Processor stepping identifier within the model.
     */
    UInt32 Stepping;
    
    /**
     * Processor model number.
     */
    UInt32 ModelNumber;
    
    /**
     * Processor family number.
     */
    UInt32 Family;
    
    /**
     * Processor type (0=OEM, 1=OverDrive, 2=Dual processor).
     */
    UInt32 ProcessorType;
    
    /**
     * Indicates if the CPU has an x87 floating-point unit on chip.
     */
    bool HasFPU;
    
    /**
     * Indicates if the CPU supports virtual 8086 mode enhancements.
     */
    bool HasVME;
    
    /**
     * Indicates if the CPU supports debugging extensions (I/O breakpoints).
     */
    bool HasDE;
    
    /**
     * Indicates if the CPU supports page size extension (4MB pages).
     */
    bool HasPSE;
    
    /**
     * Indicates if the CPU has a time stamp counter (RDTSC instruction).
     */
    bool HasTSC;
    
    /**
     * Indicates if the CPU supports model-specific registers
     * (RDMSR/WRMSR instructions).
     */
    bool HasMSR;
    
    /**
     * Indicates if the CPU supports physical address extension
     * (36-bit addressing).
     */
    bool HasPAE;
    
    /**
     * Indicates if the CPU supports machine check exception (MCE).
     */
    bool HasMCE;
    
    /**
     * Indicates if the CPU supports the CMPXCHG8B instruction.
     */
    bool HasCX8;
    
    /**
     * Indicates if the CPU has an on-chip Advanced Programmable Interrupt
     * Controller.
     */
    bool HasAPIC;
    
    /**
     * Indicates if the CPU supports SYSENTER and SYSEXIT fast system call
     * instructions.
     */
    bool HasSEP;
    
    /**
     * Indicates if the CPU supports memory type range registers for cache
     * control.
     */
    bool HasMTRR;
    
    /**
     * Indicates if the CPU supports page global enable bit in CR4.
     */
    bool HasPGE;
    
    /**
     * Indicates if the CPU supports machine check architecture.
     */
    bool HasMCA;
    
    /**
     * Indicates if the CPU supports conditional move instructions (CMOV).
     */
    bool HasCMOV;
    
    /**
     * Indicates if the CPU supports page attribute table for memory types.
     */
    bool HasPAT;
    
    /**
     * Indicates if the CPU supports 36-bit page size extension.
     */
    bool HasPSE36;
    
    /**
     * Indicates if the CPU supports the CLFLUSH cache line flush instruction.
     */
    bool HasCLFSH;
    
    /**
     * Indicates if the CPU supports MMX technology instructions.
     */
    bool HasMMX;
    
    /**
     * Indicates if the CPU supports FXSAVE and FXRSTOR instructions for FPU
     * state.
     */
    bool HasFXSR;
    
    /**
     * Indicates if the CPU supports Streaming SIMD Extensions (SSE).
     */
    bool HasSSE;
    
    /**
     * Indicates if the CPU supports Streaming SIMD Extensions 2 (SSE2).
     */
    bool HasSSE2;
    
    /**
     * Indicates if the CPU supports hyper-threading technology.
     */
    bool HasHTT;
    
    /**
     * Indicates if the CPU supports Streaming SIMD Extensions 3 (SSE3).
     */
    bool HasSSE3;
    
    /**
     * Indicates if the CPU supports the PCLMULQDQ carry-less multiplication
     * instruction.
     */
    bool HasPCLMULQDQ;
    
    /**
     * Indicates if the CPU supports Supplemental Streaming SIMD Extensions 3
     * (SSSE3).
     */
    bool HasSSSE3;
    
    /**
     * Indicates if the CPU supports fused multiply-add instructions (FMA3).
     */
    bool HasFMA;
    
    /**
     * Indicates if the CPU supports the CMPXCHG16B instruction for 16-byte
     * compare-and-swap.
     */
    bool HasCX16;
    
    /**
     * Indicates if the CPU supports Streaming SIMD Extensions 4.1 (SSE4.1).
     */
    bool HasSSE41;
    
    /**
     * Indicates if the CPU supports Streaming SIMD Extensions 4.2 (SSE4.2).
     */
    bool HasSSE42;
    
    /**
     * Indicates if the CPU supports the POPCNT population count instruction.
     */
    bool HasPOPCNT;
    
    /**
     * Indicates if the CPU supports AES instruction set for encryption.
     */
    bool HasAES;
    
    /**
     * Indicates if the CPU supports XSAVE and XRSTOR instructions for extended
     * state management.
     */
    bool HasXSAVE;
    
    /**
     * Indicates if the CPU supports Advanced Vector Extensions (AVX).
     */
    bool HasAVX;
    
    /**
     * Indicates if the CPU supports the RDRAND hardware random number generator
     * instruction.
     */
    bool HasRDRAND;
    
    /**
     * Indicates if the CPU supports RDFSBASE and WRFSBASE instructions for
     * FS/GS base access.
     */
    bool HasFSGSBASE;
    
    /**
     * Indicates if the CPU supports Bit Manipulation Instruction Set 1 (BMI1).
     */
    bool HasBMI1;
    
    /**
     * Indicates if the CPU supports Advanced Vector Extensions 2 (AVX2).
     */
    bool HasAVX2;
    
    /**
     * Indicates if the CPU supports Bit Manipulation Instruction Set 2 (BMI2).
     */
    bool HasBMI2;
    
    /**
     * Indicates if the CPU supports the RDSEED hardware random number seed
     * instruction.
     */
    bool HasRDSEED;
    
    /**
     * Indicates if the CPU supports Supervisor Mode Access Prevention.
     */
    bool HasSMAP;
    
    /**
     * Indicates if the CPU supports the optimized CLFLUSHOPT cache line flush
     * instruction.
     */
    bool HasCLFLUSHOPT;
    
    /**
     * Indicates if the CPU supports SYSCALL and SYSRET fast system call
     * instructions.
     */
    bool HasSYSCALL;
    
    /**
     * Indicates if the CPU supports the NX (No-Execute) bit for memory
     * protection.
     */
    bool HasNX;
    
    /**
     * Indicates if the CPU supports 1GB memory pages.
     */
    bool HasPage1GB;
    
    /**
     * Indicates if the CPU supports the RDTSCP instruction for reading
     * time-stamp counter.
     */
    bool HasRDTSCP;
    
    /**
     * Indicates if the CPU supports Long Mode (64-bit x86-64 architecture).
     */
    bool HasLM;
  };
}
