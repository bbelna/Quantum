/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/CPU.hpp
 * IA32 CPU control.
 */

#pragma once

#include <Arch/IA32/Types/CPU/IA32CPUInfo.hpp>
#include <CPU.hpp>
#include <Logger.hpp>
#include <Types/Logging/Level.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 CPU control.
   */
  class CPU {
    public:
      using IA32CPUInfo = Types::CPU::IA32CPUInfo;

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
       * @param physicalAddress
       *   The physical address of the page directory.
       */
      static void LoadPageDirectory(UInt32 physicalAddress);

      /**
       * Enables paging by setting the PG bit in CR0.
       */
      static void EnablePaging();

      /**
       * Invalidates a single page from the TLB.
       * @param address
       *   The virtual address of the page to invalidate.
       */
      static void InvalidatePage(UInt32 address);

      /**
       * Retrieves IA32 CPU information.
       * @return `IA32CPUInfo` structure with CPU details.
       */
      static IA32CPUInfo GetInfo();
  };
}
