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

extern "C"
__attribute__((naked))
void KernelStart() {
  Kernel::Start();
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

    "mov $0x90000, %%esp\n\t"

    // for now, no C++ call – just a visible marker
    "mov $0xB8000, %%edi\n\t"
    "mov $0x074B004F, %%eax\n\t"   // "O","K" (OK), little endian
    "mov %%eax, (%%edi)\n\t"

    "call KernelStart\n\t"
    "hlt\n\t"
    "jmp .-2\n\t"
    :
    :
    : "ax", "edi", "eax", "memory"
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
