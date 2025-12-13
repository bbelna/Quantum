//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/CPU.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// CPU control for IA32.
//------------------------------------------------------------------------------

#include <Arch/IA32/CPU.hpp>
#include <Logger.hpp>
#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using LogLevel = Logger::Level;

  void CPU::Halt() {
    asm volatile("hlt");
  }

  void CPU::HaltForever() {
    Logger::Write(LogLevel::Info, "System halted");

    for (;;) {
      asm volatile("hlt");
    }
  }

  void CPU::DisableInterrupts() {
    asm volatile("cli" ::: "memory");
  }

  void CPU::EnableInterrupts() {
    asm volatile("sti" ::: "memory");
  }

  void CPU::LoadPageDirectory(UInt32 physicalAddress) {
    asm volatile("mov %0, %%cr3" :: "r"(physicalAddress) : "memory");
  }

  void CPU::EnablePaging() {
    UInt32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // set PG bit
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
  }

  void CPU::InvalidatePage(UInt32 address) {
    asm volatile("invlpg (%0)" :: "r"(address) : "memory");
  }
}