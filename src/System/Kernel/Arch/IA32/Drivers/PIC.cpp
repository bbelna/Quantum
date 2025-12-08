//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/PIC.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// 8259A PIC implementation.
//------------------------------------------------------------------------------

#include <Arch/IA32/Drivers/PIC.hpp>
#include <Arch/IA32/Drivers/IO.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace {
    constexpr uint8 picEoi = 0x20;
    constexpr uint16 pic1Command = 0x20;
    constexpr uint16 pic1Data = 0x21;
    constexpr uint16 pic2Command = 0xA0;
    constexpr uint16 pic2Data = 0xA1;

    constexpr uint8 icw1Init = 0x10;
    constexpr uint8 icw1Icw4 = 0x01;
    constexpr uint8 icw48086 = 0x01;
  }

  void PIC::Initialize(uint8 offset1, uint8 offset2) {
    // preserve current masks so we restore them after the remap
    uint8 masterMask = IO::InByte(pic1Data);
    uint8 slaveMask = IO::InByte(pic2Data);

    // start the initialization sequence (cascade mode, expect ICW4)
    IO::OutByte(pic1Command, icw1Init | icw1Icw4);
    IO::OutByte(pic2Command, icw1Init | icw1Icw4);

    // set interrupt vector offsets
    IO::OutByte(pic1Data, offset1);
    IO::OutByte(pic2Data, offset2);

    // tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    IO::OutByte(pic1Data, 0x04);

    // tell Slave PIC its cascade identity (0000 0010)
    IO::OutByte(pic2Data, 0x02);

    // set 8086/88 mode
    IO::OutByte(pic1Data, icw48086);
    IO::OutByte(pic2Data, icw48086);

    // restore saved masks
    IO::OutByte(pic1Data, masterMask);
    IO::OutByte(pic2Data, slaveMask);
  }

  void PIC::SendEOI(uint8 irq) {
    if (irq >= 8) {
      IO::OutByte(pic2Command, picEoi);
    }

    IO::OutByte(pic1Command, picEoi);
  }

  void PIC::Mask(uint8 irq) {
    uint16 port = (irq < 8) ? pic1Data : pic2Data;
    if (irq >= 8) {
      irq -= 8;
    }

    uint8 mask = IO::InByte(port);
    mask |= static_cast<uint8>(1 << irq);
    IO::OutByte(port, mask);
  }

  void PIC::Unmask(uint8 irq) {
    uint16 port = (irq < 8) ? pic1Data : pic2Data;
    if (irq >= 8) {
      irq -= 8;
    }

    uint8 mask = IO::InByte(port);
    mask &= static_cast<uint8>(~(1 << irq));
    IO::OutByte(port, mask);
  }

  void PIC::MaskAll() {
    IO::OutByte(pic1Data, 0xFF);
    IO::OutByte(pic2Data, 0xFF);
  }
}
