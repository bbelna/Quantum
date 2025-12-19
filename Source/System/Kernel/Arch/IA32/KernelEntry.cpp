/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/KernelEntry.cpp
 * IA32 kernel entry routines.
 */

#include <Arch/IA32/BootInfo.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/KernelEntry.hpp>
#include <Arch/IA32/LinkerSymbols.hpp>
#include <Arch/IA32/TSS.hpp>
#include <Arch/IA32/VGAConsole.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Types.hpp>

using namespace Quantum::System::Kernel;

using CPU = Arch::IA32::CPU;
using VGAConsole = Arch::IA32::VGAConsole;
using LogLevel = Logger::Level;
using Writer = Logger::Writer;

/**
 * The GDT descriptor defined in the assembly GDT file.
 */
extern "C" void* GDTDescriptor32;

namespace {
  /**
   * Bootstrap page directory used before the main memory manager takes over.
   */
  [[gnu::section(".text.start.data")]]
  alignas(4096) UInt32 _bootstrapPageDirectory[1024];

  /**
   * Page tables covering the 16 MB identity window.
   */
  [[gnu::section(".text.start.data")]]
  alignas(4096) UInt32 _bootstrapPageTables[4][1024];

  /**
   * Page tables for the higher-half kernel image during bootstrap.
   */
  [[gnu::section(".text.start.data")]]
  alignas(4096) UInt32 _bootstrapKernelTables[8][1024];

  /**
   * Present page flag.
   */
  constexpr UInt32 _pagePresent = 0x1;

  /**
   * Writable page flag.
   */
  constexpr UInt32 _pageWrite = 0x2;

  /**
   * Recursive page table slot.
   */
  constexpr UInt32 _recursiveSlot = 1023;

  /**
   * IA32 page size.
   */
  constexpr UInt32 _pageSize = 4096;

  /**
   * Size of the identity-mapped window during bootstrap.
   */
  constexpr UInt32 _identityWindowBytes = 16 * 1024 * 1024;

  /**
   * Physical address of the init bundle.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInitBundlePhys = 0;

  /**
   * Size of the init bundle.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInitBundleSize = 0;

  /**
   * Magic value 0 from the init bundle header.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInitBundleMagic0 = 0;

  /**
   * Magic value 1 from the init bundle header.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInitBundleMagic1 = 0;

  /**
   * Physical address of the boot info structure.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInfoPhysical = 0;

  /**
   * Number of entries in the boot info structure.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInfoEntryCount = 0;

  /**
   * Reserved field from the boot info structure.
   */
  [[gnu::section(".text.start.data")]]
  UInt32 _bootInfoReserved = 0;

  /**
   * Builds identity and higher-half mappings needed to turn on paging.
   */
  [[gnu::section(".text.start")]]
  void BuildBootstrapPaging() {
    // clear directory
    for (UInt32 i = 0; i < 1024; ++i) {
      _bootstrapPageDirectory[i] = 0;
    }

    // identity map first 16 mb (4 tables)
    for (UInt32 t = 0; t < 4; ++t) {
      UInt32* table = _bootstrapPageTables[t];

      for (UInt32 entryIndex = 0; entryIndex < 1024; ++entryIndex) {
        UInt32 physicalAddress = (t * 1024 + entryIndex) * _pageSize;

        table[entryIndex] = physicalAddress | _pagePresent | _pageWrite;
      }

      _bootstrapPageDirectory[t]
        = reinterpret_cast<UInt32>(table)
        | _pagePresent
        | _pageWrite;
    }

    // map kernel higher-half: map the loaded higher-half image
    UInt32 kernelPhysicalStart = reinterpret_cast<UInt32>(&__hh_phys_start);
    UInt32 kernelPhysicalEnd   = reinterpret_cast<UInt32>(&__phys_end);
    UInt32 kernelImageBytes    = kernelPhysicalEnd - kernelPhysicalStart;
    UInt32 kernelVirtualBase   = reinterpret_cast<UInt32>(&__hh_virt_start);
    UInt32 nextKernelTable = 0;

    // map each page of the kernel image
    for (UInt32 offset = 0; offset < kernelImageBytes; offset += _pageSize) {
      UInt32 physicalAddress = kernelPhysicalStart + offset;
      UInt32 virtualAddress = kernelVirtualBase + offset;
      UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
      UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

      if (_bootstrapPageDirectory[pageDirectoryIndex] == 0) {
        UInt32 tablePhysical = 0;

        if (pageDirectoryIndex < 4) {
          tablePhysical = reinterpret_cast<UInt32>(
            _bootstrapPageTables[pageDirectoryIndex]
          );
        } else if (nextKernelTable < 8) {
          UInt32* table = _bootstrapKernelTables[nextKernelTable++];

          for (UInt32 i = 0; i < 1024; ++i) {
            table[i] = 0;
          }

          tablePhysical = reinterpret_cast<UInt32>(table);
        }

        _bootstrapPageDirectory[pageDirectoryIndex]
          = tablePhysical | _pagePresent | _pageWrite;
      }

      UInt32* table = reinterpret_cast<UInt32*>(
        _bootstrapPageDirectory[pageDirectoryIndex] & ~0xFFFu
      );

      table[pageTableIndex] = physicalAddress | _pagePresent | _pageWrite;
    }

    // install recursive mapping
    _bootstrapPageDirectory[_recursiveSlot]
      = reinterpret_cast<UInt32>(_bootstrapPageDirectory)
      | _pagePresent
      | _pageWrite;
  }

}

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
  _bootInfoPhysical = bootInfoPhysicalAddress;

  if (bootInfoPhysicalAddress != 0) {
    const Arch::IA32::BootInfo::Raw* bootInfo
      = reinterpret_cast<const Arch::IA32::BootInfo::Raw*>(
        bootInfoPhysicalAddress
      );

    _bootInfoEntryCount = bootInfo->entryCount;
    _bootInfoReserved = bootInfo->reserved;
    _bootInitBundlePhys = bootInfo->initBundlePhysical;
    _bootInitBundleSize = bootInfo->initBundleSize;

    if (_bootInitBundlePhys != 0 && _bootInitBundleSize >= 8) {
      const UInt8* magicBase
        = reinterpret_cast<const UInt8*>(_bootInitBundlePhys);
      _bootInitBundleMagic0 = *reinterpret_cast<const UInt32*>(magicBase);
      _bootInitBundleMagic1 = *reinterpret_cast<const UInt32*>(magicBase + 4);
    }
  } else {
    _bootInfoEntryCount = 0;
    _bootInfoReserved = 0;
    _bootInitBundlePhys = 0;
    _bootInitBundleSize = 0;
    _bootInitBundleMagic0 = 0;
    _bootInitBundleMagic1 = 0;
  }

  BuildBootstrapPaging();

  UInt32 pageDirectoryPhysical
    = reinterpret_cast<UInt32>(_bootstrapPageDirectory);

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

extern "C" [[gnu::section(".text.start.entry")]] [[gnu::naked]]
void KernelEntry() {
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
  Arch::IA32::TSS::Initialize(0);

  Initialize(bootInfoPhysicalAddress);

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

  Logger::Initialize(LogLevel::Info, writerArray, 1);

  Logger::WriteFormatted(
    LogLevel::Debug,
    "BootInfo pre-paging: addr=%p entries=%u reserved=%p",
    _bootInfoPhysical,
    _bootInfoEntryCount,
    _bootInfoReserved
  );
  Logger::WriteFormatted(
    LogLevel::Debug,
    "INIT.BND pre-paging: phys=%p size=0x%x magic0=0x%x magic1=0x%x",
    _bootInitBundlePhys,
    _bootInitBundleSize,
    _bootInitBundleMagic0,
    _bootInitBundleMagic1
  );

  if (_bootInitBundlePhys != 0 && _bootInitBundleSize >= 8) {
    const UInt8* magicBase
      = reinterpret_cast<const UInt8*>(_bootInitBundlePhys);
    UInt32 liveMagic0 = *reinterpret_cast<const UInt32*>(magicBase);
    UInt32 liveMagic1 = *reinterpret_cast<const UInt32*>(magicBase + 4);

    Logger::WriteFormatted(
      LogLevel::Debug,
      "INIT.BND live pre-mm: phys=%p magic0=0x%x magic1=0x%x",
      _bootInitBundlePhys,
      liveMagic0,
      liveMagic1
    );
  }
}
