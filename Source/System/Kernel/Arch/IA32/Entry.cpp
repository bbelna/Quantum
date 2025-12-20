/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Entry.cpp
 * IA32 kernel entry routines.
 */

#include <Arch/IA32/Bootstrap.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Entry.hpp>
#include <Arch/IA32/LinkerSymbols.hpp>
#include <Arch/IA32/TSS.hpp>
#include <Arch/IA32/VGAConsole.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Prelude.hpp>
#include <Types.hpp>

using Bootstrap = Kernel::Arch::IA32::Bootstrap;
using CPU = Kernel::Arch::IA32::CPU;
using Logger = Kernel::Logger;
using LogLevel = Logger::Level;
using TSS = Kernel::Arch::IA32::TSS;
using VGAConsole = Kernel::Arch::IA32::VGAConsole;
using Writer = Kernel::Logger::Writer;

/**
 * The GDT descriptor defined in the assembly GDT file.
 */
extern "C" void* gdtDescriptor32;

/**
 * Enables paging using the bootstrap page tables, then jumps to the higher-half
 * entry point with a higher-half stack.
 * @param bootInfoPhysicalAddress
 *   Physical address of the boot info structure.
 */
extern "C" [[noreturn]] [[gnu::section(".text.start")]]
void EnablePagingAndJump(
  UInt32 bootInfoPhysicalAddress
) {
  Bootstrap::CaptureBootInfo(bootInfoPhysicalAddress);
  Bootstrap::BuildBootstrapPaging();

  UInt32 pageDirectoryPhysical
    = Bootstrap::GetBootstrapPageDirectoryPhysical();

  asm volatile("mov %0, %%cr3" : : "r"(pageDirectoryPhysical) : "memory");

  UInt32 cr0;

  asm volatile("mov %%cr0, %0" : "=r"(cr0));

  cr0 |= 0x80000000; // enable paging

  asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

  // keep using the low stack; it remains identity mapped
  UInt32 esp;

  asm volatile("mov %%esp, %0" : "=r"(esp));

  UInt32 higherEsp = esp;

  // higher-half entry address for Start
  UInt32 startKernelHigh = reinterpret_cast<UInt32>(&Start);

  // jump to higher-half Start with adjusted stack
  asm volatile(
    "mov %0, %%esp\n\t"
    "push %1\n\t"
    "call *%2\n\t"
    :
    : "r"(higherEsp), "r"(bootInfoPhysicalAddress), "r"(startKernelHigh)
    : "memory"
  );

  for (;;) {
    asm volatile("hlt");
  }
}

extern "C" [[gnu::section(".text.start.entry")]] [[gnu::naked]]
void Entry() {
  asm volatile(
    "cli\n\t"
    "lgdt [gdtDescriptor32]\n\t"
    "mov $0x10, %%ax\n\t"
    "mov %%ax, %%ds\n\t"
    "mov %%ax, %%es\n\t"
    "mov %%ax, %%ss\n\t"
    "mov %%ax, %%fs\n\t"
    "mov %%ax, %%gs\n\t"
    "mov $0x90000, %%esp\n\t"
    "push %%esi\n\t" // bootInfoPhysicalAddress
    "call EnablePagingAndJump\n\t"
    "add $4, %%esp\n\t"
    "1:\n\t"
    "hlt\n\t"
    "jmp 1b\n\t"
    :
    :
    : "ax", "memory"
  );
}

extern "C" void Start(UInt32 bootInfoPhysicalAddress) {
  ClearBSS();
  InitializeLogging();

  CPU::GetInfo();
  TSS::Initialize(0);

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

void InitializeLogging() {
  VGAConsole::Initialize();

  static Writer* writerArray[1];

  writerArray[0] = &VGAConsole::GetWriter();

  Logger::Initialize(LogLevel::Info, writerArray, 1);

  Bootstrap::TraceBootInfo();
}
