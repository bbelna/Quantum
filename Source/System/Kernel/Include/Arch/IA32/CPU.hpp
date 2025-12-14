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

  /**
   * Low-level IA32 CPU control helpers.
   */
  class CPU {
    public:
      /**
       * Halts the CPU until the next interrupt.
       */
      static void Halt();

      /**
       * Halts the CPU forever.
       */
      [[noreturn]] static void HaltForever();

      /**
       * Disable interrupts.
       */
      static void DisableInterrupts();

      /**
       * Enable interrupts.
       */
      static void EnableInterrupts();

      /**
       * Loads the physical address of the page directory into CR3.
       */
      static void LoadPageDirectory(UInt32 physicalAddress);

      /**
       * Enables paging by setting the PG bit in CR0.
       */
      static void EnablePaging();

      /**
       * Invalidates a single page from the TLB.
       */
      static void InvalidatePage(UInt32 address);
  };
}
