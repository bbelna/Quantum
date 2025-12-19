/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/TSS.cpp
 * IA32 Task State Segment setup.
 */

#include <Arch/IA32/TSS.hpp>
#include <Arch/IA32/GDT.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  namespace {
    /**
     * Dedicated ring0 stack for privilege transitions.
     */
    alignas(16) UInt8 _ring0Stack[4096];

    /**
     * TSS instance.
     */
    TSS::Structure _tss = {};

    /**
     * GDT table provided by assembly.
     */
    extern "C" UInt64 GDT[];

    constexpr UInt32 _tssEntryIndex = 5;

    void WriteTSSDescriptor(UInt32 base, UInt32 limit) {
      GDT::Entry* entries = reinterpret_cast<GDT::Entry*>(GDT);
      GDT::Entry& tssEntry = entries[_tssEntryIndex];

      tssEntry.limitLow = static_cast<UInt16>(limit & 0xFFFF);
      tssEntry.baseLow = static_cast<UInt16>(base & 0xFFFF);
      tssEntry.baseMid = static_cast<UInt8>((base >> 16) & 0xFF);
      tssEntry.access = 0x89; // present, ring0, 32-bit available TSS
      tssEntry.granularity = static_cast<UInt8>((limit >> 16) & 0x0F);
      tssEntry.baseHigh = static_cast<UInt8>((base >> 24) & 0xFF);
    }
  }

  void TSS::Initialize(UInt32 kernelStackTop) {
    for (UInt32 i = 0; i < sizeof(TSS::Structure); ++i) {
      reinterpret_cast<UInt8*>(&_tss)[i] = 0;
    }

    if (kernelStackTop == 0) {
      kernelStackTop = reinterpret_cast<UInt32>(_ring0Stack)
        + sizeof(_ring0Stack);
    }

    _tss.ss0 = kernelDataSelector;
    _tss.esp0 = kernelStackTop;
    _tss.ioMapBase = sizeof(TSS::Structure);

    WriteTSSDescriptor(
      reinterpret_cast<UInt32>(&_tss),
      sizeof(TSS::Structure) - 1
    );

    UInt16 selector = tssSelector;

    asm volatile("ltr %0" :: "r"(selector));
  }

  void TSS::SetKernelStack(UInt32 kernelStackTop) {
    if (kernelStackTop == 0) {
      kernelStackTop = reinterpret_cast<UInt32>(_ring0Stack)
        + sizeof(_ring0Stack);
    }

    _tss.esp0 = kernelStackTop;
  }
}
