//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/CPU.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// CPU control for IA32.
//------------------------------------------------------------------------------

#pragma once

#include <Logger.hpp>
#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using LogLevel = Logger::Level;

  class CPU {
    public:
      /**
       * Halts the CPU until the next interrupt.
       */
      static inline void Halt() {
        asm volatile("hlt");
      }

      /**
       * Halts the CPU forever.
       */
      [[noreturn]] static inline void HaltForever() {
        Logger::Write(LogLevel::Info, "System halted");

        for (;;) {
          asm volatile("hlt");
        }
      }

      /**
       * Disable interrupts.
       */
      static inline void DisableInterrupts() {
        asm volatile("cli" ::: "memory");
      }

      /**
       * Enable interrupts.
       */
      static inline void EnableInterrupts() {
        asm volatile("sti" ::: "memory");
      }

      /**
       * Loads the physical address of the page directory into CR3.
       */
      static inline void LoadPageDirectory(UInt32 physAddr) {
        asm volatile("mov %0, %%cr3" :: "r"(physAddr) : "memory");
      }

      /**
       * Enables paging by setting the PG bit in CR0.
       */
      static inline void EnablePaging() {
        UInt32 cr0;
        asm volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 |= 0x80000000; // set PG bit
        asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
      }

      /**
       * Invalidates a single page from the TLB.
       */
      static inline void InvalidatePage(UInt32 addr) {
        asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
      }
  };
}
