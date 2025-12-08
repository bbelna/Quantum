//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/PIC.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Programmable Interrupt Controller (8259A) driver.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  /**
   * 8259A Programmable Interrupt Controller (PIC) helper.
   */
  class PIC {
    public:
      /**
       * Remaps the PIC to the given vector offsets and preserves masks.
       * Commonly offset1=0x20 (IRQ0) and offset2=0x28 (IRQ8).
       */
      static void Initialize(uint8 offset1, uint8 offset2);

      /**
       * Sends an End Of Interrupt (EOI) to the PICs.
       * @param irq IRQ number (0-15) that just fired.
       */
      static void SendEOI(uint8 irq);

      /**
       * Masks (disables) a specific IRQ line.
       */
      static void Mask(uint8 irq);

      /**
       * Unmasks (enables) a specific IRQ line.
       */
      static void Unmask(uint8 irq);

      /**
       * Masks all IRQ lines on both PICs.
       */
      static void MaskAll();
  };
}
