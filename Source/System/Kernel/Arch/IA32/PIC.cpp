/**
 * @file System/Kernel/Arch/IA32/PIC.cpp
 * @brief IA32 Programmable Interrupt Controller (PIC) driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/PIC.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  void PIC::Initialize(UInt8 offset1, UInt8 offset2) {
    // preserve current masks so we restore them after the remap
    UInt8 masterMask = IO::In8(_pic1Data);
    UInt8 slaveMask = IO::In8(_pic2Data);

    // start the initialization sequence (cascade mode, expect ICW4)
    IO::Out8(_pic1Command, _icw1Init | _icw1Icw4);
    IO::Out8(_pic2Command, _icw1Init | _icw1Icw4);

    // set interrupt vector offsets
    IO::Out8(_pic1Data, offset1);
    IO::Out8(_pic2Data, offset2);

    // tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    IO::Out8(_pic1Data, 0x04);

    // tell Slave PIC its cascade identity (0000 0010)
    IO::Out8(_pic2Data, 0x02);

    // set 8086/88 mode
    IO::Out8(_pic1Data, _icw48086);
    IO::Out8(_pic2Data, _icw48086);

    // restore saved masks
    IO::Out8(_pic1Data, masterMask);
    IO::Out8(_pic2Data, slaveMask);
  }

  void PIC::SendEOI(UInt8 irq) {
    if (irq >= 8) {
      IO::Out8(_pic2Command, _picEoi);
    }

    IO::Out8(_pic1Command, _picEoi);
  }

  void PIC::Mask(UInt8 irq) {
    UInt16 port = (irq < 8) ? _pic1Data : _pic2Data;

    if (irq >= 8) {
      irq -= 8;
    }

    UInt8 mask = IO::In8(port);

    mask |= static_cast<UInt8>(1 << irq);

    IO::Out8(port, mask);
  }

  void PIC::MaskAll() {
    IO::Out8(_pic1Data, 0xFF);
    IO::Out8(_pic2Data, 0xFF);
  }

  void PIC::Unmask(UInt8 irq) {
    UInt16 port = (irq < 8) ? _pic1Data : _pic2Data;

    if (irq >= 8) {
      irq -= 8;
    }

    UInt8 mask = IO::In8(port);

    mask &= static_cast<UInt8>(~(1 << irq));

    IO::Out8(port, mask);
  }

  void PIC::UnmaskAll() {
    IO::Out8(_pic1Data, 0x00);
    IO::Out8(_pic2Data, 0x00);
  }
}
