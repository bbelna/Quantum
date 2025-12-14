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
#include <Arch/IA32/Memory.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Types.hpp>
#include <Types/Writer.hpp>

using namespace Quantum::Kernel;

using CPU = Arch::IA32::CPU;
using LogLevel = Logger::Level;
using VGAConsole = Arch::IA32::Drivers::VGAConsole;
using Writer = Types::Writer;

extern "C" void* GDTDescriptor32;

namespace {
  /**
   * Bootstrap page directory used before the main memory manager takes over.
   */
  alignas(4096) UInt32 bootstrapPageDirectory[1024] __attribute__((section(".text.start.data")));

  /**
   * Page tables covering the 16 MB identity window.
   */
  alignas(4096) UInt32 bootstrapPageTables[4][1024] __attribute__((section(".text.start.data")));

  /**
   * Page tables for the higher-half kernel image during bootstrap.
   */
  alignas(4096) UInt32 bootstrapKernelTables[8][1024] __attribute__((section(".text.start.data")));

  constexpr UInt32 pagePresent = 0x1;
  constexpr UInt32 pageWrite = 0x2;
  constexpr UInt32 recursiveSlot = 1023;
  constexpr UInt32 pageSize = 4096;
  constexpr UInt32 identityWindowBytes = 16 * 1024 * 1024;

  /**
   * Builds identity and higher-half mappings needed to turn on paging.
   */
  __attribute__((section(".text.start")))
  void BuildBootstrapPaging() {
    // clear directory
    for (UInt32 i = 0; i < 1024; ++i) {
      bootstrapPageDirectory[i] = 0;
    }

    // identity map first 16 mb (4 tables)
    for (UInt32 t = 0; t < 4; ++t) {
      UInt32* table = bootstrapPageTables[t];

      for (UInt32 entryIndex = 0; entryIndex < 1024; ++entryIndex) {
        UInt32 physicalAddress = (t * 1024 + entryIndex) * pageSize;

        table[entryIndex] = physicalAddress | pagePresent | pageWrite;
      }

      bootstrapPageDirectory[t] = reinterpret_cast<UInt32>(table) | pagePresent | pageWrite;
    }

    // map kernel higher-half: map the loaded higher-half image
    // [__hh_phys_start, __phys_end) at [__hh_virt_start, __hh_virt_start + size)
    UInt32 kernelPhysicalStart = reinterpret_cast<UInt32>(&__hh_phys_start);
    UInt32 kernelPhysicalEnd   = reinterpret_cast<UInt32>(&__phys_end);
    UInt32 kernelImageBytes    = kernelPhysicalEnd - kernelPhysicalStart;
    UInt32 kernelVirtualBase   = reinterpret_cast<UInt32>(&__hh_virt_start);
    UInt32 nextKernelTable = 0;

    // map each page of the kernel image
    for (UInt32 offset = 0; offset < kernelImageBytes; offset += pageSize) {
      UInt32 physicalAddress = kernelPhysicalStart + offset;
      UInt32 virtualAddress = kernelVirtualBase + offset;
      UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
      UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

      if (bootstrapPageDirectory[pageDirectoryIndex] == 0) {
        UInt32 tablePhysical = 0;

        if (pageDirectoryIndex < 4) {
          tablePhysical = reinterpret_cast<UInt32>(bootstrapPageTables[pageDirectoryIndex]);
        } else if (nextKernelTable < 8) {
          UInt32* table = bootstrapKernelTables[nextKernelTable++];

          for (UInt32 i = 0; i < 1024; ++i) {
            table[i] = 0;
          }

          tablePhysical = reinterpret_cast<UInt32>(table);
        }

        bootstrapPageDirectory[pageDirectoryIndex] = tablePhysical | pagePresent | pageWrite;
      }

      UInt32* table = reinterpret_cast<UInt32*>(bootstrapPageDirectory[pageDirectoryIndex] & ~0xFFFu);

      table[pageTableIndex] = physicalAddress | pagePresent | pageWrite;
    }

    // install recursive mapping
    bootstrapPageDirectory[recursiveSlot]
      = reinterpret_cast<UInt32>(bootstrapPageDirectory) | pagePresent | pageWrite;
  }

}

/**
 * Enables paging using the bootstrap page tables, then jumps to the higher-half
 * entry point with a higher-half stack.
 */
extern "C" [[noreturn]] __attribute__((section(".text.start")))
void EnablePagingAndJump(UInt32 bootInfoPhysicalAddress) {
  BuildBootstrapPaging();

  UInt32 pageDirectoryPhysical = reinterpret_cast<UInt32>(bootstrapPageDirectory);

  asm volatile("mov %0, %%cr3" : : "r"(pageDirectoryPhysical) : "memory");

  UInt32 cr0;

  asm volatile("mov %%cr0, %0" : "=r"(cr0));

  cr0 |= 0x80000000; // enable paging

  asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

  // keep using the low stack; it remains identity mapped
  UInt32 esp;

  asm volatile("mov %%esp, %0" : "=r"(esp));

  UInt32 higherEsp = esp;

  // higher-half entry address for StartKernel
  UInt32 startKernelHigh = reinterpret_cast<UInt32>(&StartKernel);

  // jump to higher-half StartKernel with adjusted stack
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

extern "C" __attribute__((naked, section(".text.start.entry"))) void KernelEntry() {
  asm volatile(
    "cli\n\t"
    "lgdt [GDTDescriptor32]\n\t"
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

extern "C" void StartKernel(UInt32 bootInfoPhysicalAddress) {
  ClearBSS();
  InitializeKernelLogging();

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

  static Writer* writerArray[1];
  writerArray[0] = &VGAConsole::GetWriter();

  Logger::Initialize(LogLevel::Trace, writerArray, 1);
}
