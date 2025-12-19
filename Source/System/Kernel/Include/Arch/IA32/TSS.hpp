/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/TSS.hpp
 * IA32 Task State Segment setup.
 */

#pragma once

#include <Types/Primitives.hpp>

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
        UInt32 PreviousTSS;
        UInt32 ESP0;
        UInt32 SS0;
        UInt32 ESP1;
        UInt32 SS1;
        UInt32 ESP2;
        UInt32 SS2;
        UInt32 CR3;
        UInt32 EIP;
        UInt32 EFlags;
        UInt32 EAX;
        UInt32 ECX;
        UInt32 EDX;
        UInt32 EBX;
        UInt32 ESP;
        UInt32 EBP;
        UInt32 ESI;
        UInt32 EDI;
        UInt32 ES;
        UInt32 CS;
        UInt32 SS;
        UInt32 DS;
        UInt32 FS;
        UInt32 GS;
        UInt32 LDT;
        UInt16 Trap;
        UInt16 IOMapBase;
      };

      /**
       * Kernel code segment selector.
       */
      static constexpr UInt16 KernelCodeSelector = 0x08;

      /**
       * Kernel data segment selector.
       */
      static constexpr UInt16 KernelDataSelector = 0x10;

      /**
       * User code segment selector (RPL=3).
       */
      static constexpr UInt16 UserCodeSelector = 0x1B;

      /**
       * User data segment selector (RPL=3).
       */
      static constexpr UInt16 UserDataSelector = 0x23;

      /**
       * TSS segment selector.
       */
      static constexpr UInt16 TSSSelector = 0x28;

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
  };
}
