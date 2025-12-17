/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/PIC.cpp
 * 8259A Programmable Interrupt Controller (PIC) driver.
 */

#include <Arch/IA32/IO.hpp>
#include <Arch/IA32/PIC.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  namespace {
    /**
     * PIC EOI command value.
     */
    constexpr UInt8 _picEoi = 0x20;

    /**
     * PIC 1 command port.
     */
    constexpr UInt16 _pic1Command = 0x20;

    /**
     * PIC 1 data port.
     */
    constexpr UInt16 _pic1Data = 0x21;

    /**
     * PIC 2 command port.
     */
    constexpr UInt16 _pic2Command = 0xA0;

    /**
     * PIC 2 data port.
     */
    constexpr UInt16 _pic2Data = 0xA1;

    /**
     * ICW1 initialization command.
     */
    constexpr UInt8 _icw1Init = 0x10;

    /**
     * ICW1 expects ICW4.
     */
    constexpr UInt8 _icw1Icw4 = 0x01;

    /**
     * ICW4 mode 8086/88.
     */
    constexpr UInt8 _icw48086 = 0x01;
  }

  void PIC::Initialize(UInt8 offset1, UInt8 offset2) {
    // preserve current masks so we restore them after the remap
    UInt8 masterMask = IO::InByte(_pic1Data);
    UInt8 slaveMask = IO::InByte(_pic2Data);

    // start the initialization sequence (cascade mode, expect ICW4)
    IO::OutByte(_pic1Command, _icw1Init | _icw1Icw4);
    IO::OutByte(_pic2Command, _icw1Init | _icw1Icw4);

    // set interrupt vector offsets
    IO::OutByte(_pic1Data, offset1);
    IO::OutByte(_pic2Data, offset2);

    // tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    IO::OutByte(_pic1Data, 0x04);

    // tell Slave PIC its cascade identity (0000 0010)
    IO::OutByte(_pic2Data, 0x02);

    // set 8086/88 mode
    IO::OutByte(_pic1Data, _icw48086);
    IO::OutByte(_pic2Data, _icw48086);

    // restore saved masks
    IO::OutByte(_pic1Data, masterMask);
    IO::OutByte(_pic2Data, slaveMask);
  }

  void PIC::SendEOI(UInt8 irq) {
    if (irq >= 8) {
      IO::OutByte(_pic2Command, _picEoi);
    }

    IO::OutByte(_pic1Command, _picEoi);
  }

  void PIC::Mask(UInt8 irq) {
    UInt16 port = (irq < 8) ? _pic1Data : _pic2Data;
    if (irq >= 8) {
      irq -= 8;
    }

    UInt8 mask = IO::InByte(port);
    mask |= static_cast<UInt8>(1 << irq);
    IO::OutByte(port, mask);
  }

  void PIC::Unmask(UInt8 irq) {
    UInt16 port = (irq < 8) ? _pic1Data : _pic2Data;
    if (irq >= 8) {
      irq -= 8;
    }

    UInt8 mask = IO::InByte(port);
    mask &= static_cast<UInt8>(~(1 << irq));
    IO::OutByte(port, mask);
  }

  void PIC::MaskAll() {
    IO::OutByte(_pic1Data, 0xFF);
    IO::OutByte(_pic2Data, 0xFF);
  }
}
