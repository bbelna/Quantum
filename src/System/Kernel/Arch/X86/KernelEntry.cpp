//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Arch/x86/KernelEntry.cpp
// 32-bit entry point that calls starts the kernel.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Kernel.hpp>
#include <KernelTypes.hpp>
#include <Arch/x86/KernelEntry.hpp>

using namespace Quantum::Kernel;

extern "C" uint8 __bss_start;
extern "C" uint8 __bss_end;

extern "C"
void KernelStart() {
  uint8* bss = &__bss_start;
  uint8* bss_end = &__bss_end;
  while (bss < bss_end) {
    *bss++ = 0;
  }

  Kernel::Initialize();

  while (true) {
    asm volatile("hlt");
  }
}

extern "C"
__attribute__((naked, section(".text.start")))
void StartKernel() {
  asm volatile(
    "cli\n\t"
    "mov $0x10, %%ax\n\t"
    "mov %%ax, %%ds\n\t"
    "mov %%ax, %%es\n\t"
    "mov %%ax, %%ss\n\t"
    "mov %%ax, %%fs\n\t"
    "mov %%ax, %%gs\n\t"
    "mov $0x90000, %%esp\n\t"   // set stack BEFORE any C code runs
    "call KernelStart\n\t"
    "1:\n\t"
    "hlt\n\t"
    "jmp 1b\n\t"
    :
    :
    : "ax", "memory"
  );
}


__attribute__((aligned(16), section(".rodata")))
const uint64 GDTTable32[] = {
  0x0000000000000000ULL,    // null
  0x00AF9A000000FFFFULL,    // code segment
  0x00AF92000000FFFFULL,    // data segment
};

__attribute__((aligned(1), section(".rodata")))
struct {
  uint16 limit;
  uint32 base;
} GDTDescriptor32 = {
  sizeof(GDTTable32) - 1,
  static_cast<uint32>(
    reinterpret_cast<uintptr>(&GDTTable32)
  )
};
