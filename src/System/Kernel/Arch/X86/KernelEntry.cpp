//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Arch/x86/KernelEntry.cpp
// 32-bit entry point that calls starts the kernel.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Kernel.hpp>
#include <KernelTypes.hpp>
#include <Arch/X86/KernelEntry.hpp>

using namespace Quantum::Kernel;

extern "C" void Kernel_Start() {
  Kernel::Start();
}

__attribute__((naked, section(".text")))
void EnterProtectedMode() {
  asm volatile(
    "cli\n\t"
    "xor %ax, %ax\n\t"
    "mov %ax, %ds\n\t"
    "mov %ax, %es\n\t"
    "mov %ax, %ss\n\t"
    "mov $0x7C00, %sp\n\t"
    "lgdtl GDTDescriptor32\n\t"
    "mov %cr0, %eax\n\t"
    "or $1, %eax\n\t"
    "mov %eax, %cr0\n\t"
    "jmp $0x08, $StartKernel\n\t"
  );
}

__attribute__((naked, section(".text")))
void StartKernel() {
  asm volatile(
    "mov $0x10, %ax\n\t"
    "mov %ax, %ds\n\t"
    "mov %ax, %es\n\t"
    "mov %ax, %ss\n\t"
    "mov $0x9000, %esp\n\t"
    "sti\n\t"
    "call Kernel_Start\n\t"
    "hlt\n\t"
    "jmp  .\n\t"
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
