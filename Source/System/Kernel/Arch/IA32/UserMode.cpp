/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/UserMode.cpp
 * IA32 user-mode entry helpers.
 */

#include <Arch/IA32/Memory.hpp>
#include <Arch/IA32/TSS.hpp>
#include <Arch/IA32/UserMode.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  namespace {
    constexpr UInt32 _pageSize = 4096;

    inline UInt32 AlignUp(UInt32 value, UInt32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }
  }

  void UserMode::Enter(UInt32 entryPoint, UInt32 userStackTop) {
    const UInt32 userData = TSS::UserDataSelector;
    const UInt32 userCode = TSS::UserCodeSelector;

    asm volatile(
      "cli\n\t"
      "mov %0, %%eax\n\t"
      "mov %%ax, %%ds\n\t"
      "mov %%ax, %%es\n\t"
      "mov %%ax, %%fs\n\t"
      "mov %%ax, %%gs\n\t"
      "pushl %0\n\t"          // SS
      "pushl %1\n\t"          // ESP
      "pushf\n\t"
      "pop %%eax\n\t"
      "or $0x200, %%eax\n\t"  // IF
      "pushl %%eax\n\t"
      "pushl %2\n\t"          // CS
      "pushl %3\n\t"          // EIP
      "iret\n\t"
      :
      : "r"(userData), "r"(userStackTop), "r"(userCode), "r"(entryPoint)
      : "eax", "memory"
    );

    __builtin_unreachable();
  }

  bool UserMode::MapUserStack(UInt32 userStackTop, UInt32 sizeBytes) {
    if (sizeBytes == 0) {
      return false;
    }

    UInt32 alignedSize = AlignUp(sizeBytes, _pageSize);
    UInt32 stackBase = userStackTop - alignedSize;
    UInt32 pages = alignedSize / _pageSize;

    for (UInt32 i = 0; i < pages; ++i) {
      void* phys = Memory::AllocatePage(true);
      UInt32 vaddr = stackBase + i * _pageSize;

      Memory::MapPage(vaddr, reinterpret_cast<UInt32>(phys), true, true, false);
    }

    return true;
  }
}
