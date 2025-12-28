/**
 * @file System/Kernel/Include/Arch/IA32/TSS.hpp
 * @brief IA32 Task State Segment (TSS) implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 Task State Segment setup utilities.
   */
  class TSS {
    public:
      /**
       * IA32 TSS structure.
       */
      struct [[gnu::packed]] Structure {
        /**
         * Previous TSS selector.
         */
        UInt32 previousTSS;

        /**
         * Stack pointer for privilege level 0.
         */
        UInt32 esp0;

        /**
         * Stack segment selector for privilege level 0.
         */
        UInt32 ss0;

        /**
         * Stack pointer for privilege level 1.
         */
        UInt32 esp1;

        /**
         * Stack segment selector for privilege level 1.
         */
        UInt32 ss1;

        /**
         * Stack pointer for privilege level 2.
         */
        UInt32 esp2;

        /**
         * Stack segment selector for privilege level 2.
         */
        UInt32 ss2;

        /**
         * Control register CR3.
         */
        UInt32 cr3;

        /**
         * Extended instruction pointer register.
         */
        UInt32 eip;

        /**
         * EFLAGS register.
         */
        UInt32 eflags;

        /**
         * Extended accumulator register.
         */
        UInt32 eax;

        /**
         * Extended counter register.
         */
        UInt32 ecx;

        /**
         * Extended data register.
         */
        UInt32 edx;

        /**
         * Extended base register.
         */
        UInt32 ebx;

        /**
         * Extended stack pointer register.
         */
        UInt32 esp;

        /**
         * Extended base pointer register.
         */
        UInt32 ebp;

        /**
         * Extended source index register.
         */
        UInt32 esi;

        /**
         * Extended destination index register.
         */
        UInt32 edi;

        /**
         * Extra segment selector.
         */
        UInt32 es;

        /**
         * Code segment selector.
         */
        UInt32 cs;

        /**
         * Stack segment selector.
         */
        UInt32 ss;

        /**
         * Data segment selector.
         */
        UInt32 ds;

        /**
         * FS segment register.
         */
        UInt32 fs;

        /**
         * GS segment register.
         */
        UInt32 gs;

        /**
         * Local descriptor table segment selector.
         */
        UInt32 ldt;

        /**
         * Trap flag.
         */
        UInt16 trap;

        /**
         * I/O map base address.
         */
        UInt16 ioMapBase;
      };

      /**
       * Kernel code segment selector.
       */
      static constexpr UInt16 kernelCodeSelector = 0x08;

      /**
       * Kernel data segment selector.
       */
      static constexpr UInt16 kernelDataSelector = 0x10;

      /**
       * User code segment selector (RPL=3).
       */
      static constexpr UInt16 userCodeSelector = 0x1B;

      /**
       * User data segment selector (RPL=3).
       */
      static constexpr UInt16 userDataSelector = 0x23;

      /**
       * TSS segment selector.
       */
      static constexpr UInt16 tssSelector = 0x28;

      /**
       * Initializes the TSS and loads it into TR.
       * @param kernelStackTop
       *   Top of the ring0 stack for privilege transitions. If zero, a
       *   dedicated internal stack is used.
       */
      static void Initialize(UInt32 kernelStackTop);

      /**
       * Updates the ring0 stack pointer used for privilege transitions.
       * @param kernelStackTop
       *   Top of the ring0 stack.
       */
      static void SetKernelStack(UInt32 kernelStackTop);

    private:
      /**
       * Dedicated ring0 stack for privilege transitions.
       */
      static UInt8 _ring0Stack[4096];

      /**
       * TSS instance.
       */
      inline static Structure _tss = {};

      /**
       * Index of the TSS descriptor in the GDT.
       */
      static constexpr UInt32 _tssEntryIndex = 5;

      /**
       * Writes the TSS descriptor into the GDT.
       * @param base
       *   Base address of the TSS.
       * @param limit
       *   Limit of the TSS segment.
       */
      static void WriteTSSDescriptor(UInt32 base, UInt32 limit);
  };
}
