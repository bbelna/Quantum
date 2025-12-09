//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/KernelEntry.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 entry point that calls starts the kernel.
//------------------------------------------------------------------------------

#include <Kernel.hpp>
#include <KernelTypes.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/KernelEntry.hpp>

using namespace Quantum::Kernel;
using Quantum::Kernel::Arch::IA32::CPU;

extern "C" uint8 __bss_start;
extern "C" uint8 __bss_end;

extern "C" void* GDTDescriptor32;

extern "C" __attribute__((naked, section(".text.start"))) void KernelEntry() {
  asm volatile(
    "cli\n\t"
    "lgdt GDTDescriptor32\n\t"
    "mov $0x10, %%ax\n\t"
    "mov %%ax, %%ds\n\t"
    "mov %%ax, %%es\n\t"
    "mov %%ax, %%ss\n\t"
    "mov %%ax, %%fs\n\t"
    "mov %%ax, %%gs\n\t"
    "mov $0x90000, %%esp\n\t"
    "push %%esi\n\t"
    "call StartKernel\n\t"
    "add $4, %%esp\n\t"
    "1:\n\t"
    "hlt\n\t"
    "jmp 1b\n\t"
    :
    :
    : "ax", "memory"
  );
}

extern "C" void StartKernel(uint32 bootInfoPhys) {
  ClearBSS();

  Kernel::Initialize(bootInfoPhys);

  CPU::HaltForever();
}

void ClearBSS() {
  uint8* bss = &__bss_start;
  uint8* bss_end = &__bss_end;
  while (bss < bss_end) {
    *bss++ = 0;
  }
}
