/**
 * @file System/Kernel/Include/Arch/IA32/TSS.hpp
 * @brief IA32 Task State Segment (TSS) implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
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
        UInt32 previousTSS;
        UInt32 esp0;
        UInt32 ss0;
        UInt32 esp1;
        UInt32 ss1;
        UInt32 esp2;
        UInt32 ss2;
        UInt32 cr3;
        UInt32 eip;
        UInt32 eflags;
        UInt32 eax;
        UInt32 ecx;
        UInt32 edx;
        UInt32 ebx;
        UInt32 esp;
        UInt32 ebp;
        UInt32 esi;
        UInt32 edi;
        UInt32 es;
        UInt32 cs;
        UInt32 ss;
        UInt32 ds;
        UInt32 fs;
        UInt32 gs;
        UInt32 ldt;
        UInt16 trap;
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
