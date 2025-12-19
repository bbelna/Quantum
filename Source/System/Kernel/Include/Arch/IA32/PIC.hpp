/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/PIC.hpp
 * 8259A Programmable Interrupt Controller (PIC) driver.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * 8259A Programmable Interrupt Controller (PIC) driver.
   */
  class PIC {
    public:
      /**
       * Remaps the PIC to the given vector offsets and preserves masks.
       * Commonly offset1=0x20 (IRQ0) and offset2=0x28 (IRQ8).
       * @param offset1
       *   Vector offset for master PIC.
       * @param offset2
       *   Vector offset for slave PIC.
       */
      static void Initialize(UInt8 offset1, UInt8 offset2);

      /**
       * Sends an End Of Interrupt (EOI) to the PICs.
       * @param irq
       *   IRQ number (0-15) that just fired.
       */
      static void SendEOI(UInt8 irq);

      /**
       * Masks (disables) a specific IRQ line.
       * @param irq
       *   IRQ number (0-15) to mask.
       */
      static void Mask(UInt8 irq);

      /**
       * Unmasks (enables) a specific IRQ line.
       * @param irq
       *   IRQ number (0-15) to unmask.
       */
      static void Unmask(UInt8 irq);

      /**
       * Masks all IRQ lines on both PICs.
       */
      static void MaskAll();
  };
}
