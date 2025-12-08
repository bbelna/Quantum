//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Memory.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Architecture-agnostic memory manager entry points.
//------------------------------------------------------------------------------

#include <Memory.hpp>
#include <KernelTypes.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Memory.hpp>
  #include <Arch/IA32/CPU.hpp>

  namespace ArchMemory = Quantum::Kernel::Arch::IA32;
  using ArchCPU = Quantum::Kernel::Arch::IA32::CPU;
#else
  #error "No architecture selected for memory manager"
#endif

#include <Drivers/Console.hpp>

namespace Quantum::Kernel {
  namespace {
    // Simple bump heap mapped contiguously in virtual space.
    constexpr uint32 heapPageSize  = 4096;
    constexpr uint32 heapStartVirtual = 0x00400000; // 4 MB virtual base

    uint8* heapBase = nullptr;
    uint8* heapEnd = nullptr;
    uint8* heapCurrent = nullptr;

    inline uint32 AlignUp(uint32 value, uint32 align) {
      return (value + align - 1) & ~(align - 1);
    }

    void EnsureHeapSpace(uint32 size) {
      if (!heapBase) {
        heapBase = reinterpret_cast<uint8*>(heapStartVirtual);
        heapCurrent = heapBase;
        heapEnd = heapBase;
      }

      while (heapCurrent + size > heapEnd) {
        void* phys = ArchMemory::AllocatePage();
        ArchMemory::MapPage(
          reinterpret_cast<uint32>(heapEnd),
          reinterpret_cast<uint32>(phys),
          true
        );
        heapEnd += heapPageSize;
      }
    }
  }

  void Memory::Initialize() {
    ArchMemory::InitializePaging();
  }

  void* Memory::AllocatePage() {
    return ArchMemory::AllocatePage();
  }

  void* Memory::Allocate(usize size) {
    // 8-byte align for basic pointer safety
    uint32 alignedSize = AlignUp(static_cast<uint32>(size), 8);
    EnsureHeapSpace(alignedSize);

    void* ptr = heapCurrent;
    heapCurrent += alignedSize;

    // extremely simple; no freeing support
    if (heapCurrent > heapEnd) {
      Drivers::Console::WriteLine("Kernel heap exhausted");
      ArchCPU::HaltForever();
    }

    return ptr;
  }
}
