//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/CPU.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// CPU control for IA32.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>
#include <Drivers/Console.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using Quantum::Kernel::Drivers::Console;

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
        Console::WriteLine("System halted");

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
  };
}
