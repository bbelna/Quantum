//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/KernelEntry.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// IA32 entry point that calls starts the kernel.
//------------------------------------------------------------------------------

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Drivers/VGAConsole.hpp>
#include <Arch/IA32/KernelEntry.hpp>
#include <Arch/IA32/LinkerSymbols.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Types.hpp>
#include <Types/Writer.hpp>

using namespace Quantum::Kernel;

using CPU = Arch::IA32::CPU;
using LogLevel = Logger::Level;
using VGAConsole = Arch::IA32::Drivers::VGAConsole;
using Writer = Types::Writer;

extern "C" void* GDTDescriptor32;

extern "C" __attribute__((naked, section(".text.start"))) void KernelEntry() {
  asm volatile(
    "cli\n\t"
    "lgdt GDTDescriptor32\n\t"
    "mov $0x10, %%ax\n\t"
    "mov %%ax, %%ds\n\t"
    "mov %%ax, %%es\n\t"
    "mov %%ax, %%ss\n\t"
    "mov %%ax, %%fs\n\t"
    "mov %%ax, %%gs\n\t"
    "mov $0x90000, %%esp\n\t"
    "push %%esi\n\t"
    "call StartKernel\n\t"
    "add $4, %%esp\n\t"
    "1:\n\t"
    "hlt\n\t"
    "jmp 1b\n\t"
    :
    :
    : "ax", "memory"
  );
}

extern "C" void StartKernel(UInt32 bootInfoPhysicalAddress) {
  ClearBSS();
  InitializeKernelLogging();
  TraceVersionAndCopyright();

  Kernel::Initialize(bootInfoPhysicalAddress);

  PANIC("Returned from Kernel::Initialize()");
}

void ClearBSS() {
  UInt8* bss = &__bss_start;
  UInt8* bss_end = &__bss_end;

  while (bss < bss_end) {
    *bss++ = 0;
  }
}

void InitializeKernelLogging() {
  VGAConsole::Initialize();

  Writer* vgaWriter = &VGAConsole::GetWriter();
  Writer** writers = &vgaWriter;

  Logger::Initialize(LogLevel::Trace, writers, 1);
}

void TraceVersionAndCopyright() {
  // TODO: versioning, build date, etc.
  Logger::Write(LogLevel::Info, "x86 Quantum Kernel");
  Logger::Write(LogLevel::Info, "Copyright (c) 2025 Brandon Belna");
  Logger::Write(LogLevel::Info, "Released under the MIT License");
  Logger::Write(LogLevel::Info, "Provided \"AS IS\" without warranty");
}