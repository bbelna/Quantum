/**
 * @file System/Kernel/Include/Arch/IA32/PIC.hpp
 * @brief IA32 Programmable Interrupt Controller (PIC) driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
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
       * Masks all IRQ lines on both PICs.
       */
      static void MaskAll();

      /**
       * Unmasks (enables) a specific IRQ line.
       * @param irq
       *   IRQ number (0-15) to unmask.
       */
      static void Unmask(UInt8 irq);

      /**
       * Unmasks all IRQ lines on both PICs.
       */
      static void UnmaskAll();

    private:
      /**
       * PIC EOI command value.
       */
      static constexpr UInt8 _picEoi = 0x20;

      /**
       * PIC 1 command port.
       */
      static constexpr UInt16 _pic1Command = 0x20;

      /**
       * PIC 1 data port.
       */
      static constexpr UInt16 _pic1Data = 0x21;

      /**
       * PIC 2 command port.
       */
      static constexpr UInt16 _pic2Command = 0xA0;

      /**
       * PIC 2 data port.
       */
      static constexpr UInt16 _pic2Data = 0xA1;

      /**
       * ICW1 initialization command.
       */
      static constexpr UInt8 _icw1Init = 0x10;

      /**
       * ICW1 expects ICW4.
       */
      static constexpr UInt8 _icw1Icw4 = 0x01;

      /**
       * ICW4 mode 8086/88.
       */
      static constexpr UInt8 _icw48086 = 0x01;
  };
}
