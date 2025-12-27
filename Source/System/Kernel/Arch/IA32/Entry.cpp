/**
 * @file System/Kernel/Arch/IA32/Entry.cpp
 * @brief IA32 kernel entry.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/BootInfo.hpp"
#include "Arch/IA32/Bootstrap.hpp"
#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Entry.hpp"
#include "Arch/IA32/LinkerSymbols.hpp"
#include "Arch/IA32/Prelude.hpp"
#include "Arch/IA32/TSS.hpp"
#include "Console.hpp"
#include "Logger.hpp"
#include "Main.hpp"
#include "Memory.hpp"
#include "Panic.hpp"
#include "Prelude.hpp"

using BootInfoRaw = KernelIA32::BootInfo::Raw;
using Bootstrap = KernelIA32::Bootstrap;
using Console = Kernel::Console;
using CPU = KernelIA32::CPU;
using Logger = Kernel::Logger;
using LogLevel = Kernel::Logger::Level;
using TSS = KernelIA32::TSS;
using Writer = Kernel::Logger::Writer;

/**
 * The GDT descriptor defined in the assembly GDT file.
 */
extern "C" void* gdtDescriptor32;

void ClearBSS();
void InitializeLogging();
void RelocateInitBundle(UInt32 bootInfoPhysicalAddress);

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
  RelocateInitBundle(bootInfoPhysicalAddress);
  ClearBSS();
  InitializeLogging();

  TSS::Initialize(0);

  Kernel::Main(bootInfoPhysicalAddress);

  PANIC("Returned from Main()");
}

void ClearBSS() {
  UInt8* bss = &__bss_start;
  UInt8* bss_end = &__bss_end;

  while (bss < bss_end) {
    *bss++ = 0;
  }
}

void RelocateInitBundle(UInt32 bootInfoPhysicalAddress) {
  if (bootInfoPhysicalAddress != 0) {
    BootInfoRaw* bootInfo
      = reinterpret_cast<BootInfoRaw*>(bootInfoPhysicalAddress);

    if (bootInfo->initBundlePhysical != 0 && bootInfo->initBundleSize != 0) {
      constexpr UInt32 newBase = 0x00200000;
      UInt32 size = bootInfo->initBundleSize;
      UInt8* src = reinterpret_cast<UInt8*>(bootInfo->initBundlePhysical);
      UInt8* dst = reinterpret_cast<UInt8*>(newBase);

      if (bootInfo->initBundlePhysical != newBase) {
        for (UInt32 i = 0; i < size; ++i) {
          dst[i] = src[i];
        }

        bootInfo->initBundlePhysical = newBase;
      }
    }
  }
}

void InitializeLogging() {
  Console::Initialize();

  static Writer* writerArray[1];

  writerArray[0] = &Console::GetWriter();

  Logger::Initialize(LogLevel::Info, writerArray, 1);

  Bootstrap::TraceBootInfo();
}
