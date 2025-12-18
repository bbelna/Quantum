/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/TSS.cpp
 * IA32 Task State Segment setup.
 */

#include <Arch/IA32/TSS.hpp>
#include <Arch/IA32/Types/GDT/GDTEntry.hpp>
#include <Arch/IA32/Types/Tasks/TSS.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  namespace {
    using GDTEntry = Types::GDT::GDTEntry;
    using TSS32 = Types::Tasks::TSS32;

    /**
     * Dedicated ring0 stack for privilege transitions.
     */
    alignas(16) UInt8 _ring0Stack[4096];

    /**
     * TSS instance.
     */
    TSS32 _tss = {};

    /**
     * GDT table provided by assembly.
     */
    extern "C" UInt64 GDT[];

    constexpr UInt32 _tssEntryIndex = 5;

    void WriteTSSDescriptor(UInt32 base, UInt32 limit) {
      GDTEntry* entries = reinterpret_cast<GDTEntry*>(GDT);
      GDTEntry& tssEntry = entries[_tssEntryIndex];

      tssEntry.LimitLow = static_cast<UInt16>(limit & 0xFFFF);
      tssEntry.BaseLow = static_cast<UInt16>(base & 0xFFFF);
      tssEntry.BaseMid = static_cast<UInt8>((base >> 16) & 0xFF);
      tssEntry.Access = 0x89; // present, ring0, 32-bit available TSS
      tssEntry.Granularity = static_cast<UInt8>((limit >> 16) & 0x0F);
      tssEntry.BaseHigh = static_cast<UInt8>((base >> 24) & 0xFF);
    }
  }

  void TSS::Initialize(UInt32 kernelStackTop) {
    for (UInt32 i = 0; i < sizeof(TSS32); ++i) {
      reinterpret_cast<UInt8*>(&_tss)[i] = 0;
    }

    if (kernelStackTop == 0) {
      kernelStackTop = reinterpret_cast<UInt32>(_ring0Stack)
        + sizeof(_ring0Stack);
    }

    _tss.SS0 = KernelDataSelector;
    _tss.ESP0 = kernelStackTop;
    _tss.IOMapBase = sizeof(TSS32);

    WriteTSSDescriptor(
      reinterpret_cast<UInt32>(&_tss),
      sizeof(TSS32) - 1
    );

    UInt16 selector = TSSSelector;

    asm volatile("ltr %0" :: "r"(selector));
  }

  void TSS::SetKernelStack(UInt32 kernelStackTop) {
    if (kernelStackTop == 0) {
      kernelStackTop = reinterpret_cast<UInt32>(_ring0Stack)
        + sizeof(_ring0Stack);
    }

    _tss.ESP0 = kernelStackTop;
  }
}
