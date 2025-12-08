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
    constexpr uint16 PIC1_COMMAND = 0x20;
    constexpr uint16 PIC1_DATA = 0x21;
    constexpr uint16 PIC2_COMMAND = 0xA0;
    constexpr uint16 PIC2_DATA = 0xA1;

    constexpr uint8 PIC_EOI = 0x20;
    constexpr uint8 ICW1_INIT = 0x10;
    constexpr uint8 ICW1_ICW4 = 0x01;
    constexpr uint8 ICW4_8086 = 0x01;
  }

  void PIC::Initialize(uint8 offset1, uint8 offset2) {
    // preserve current masks so we restore them after the remap
    uint8 masterMask = IO::InByte(PIC1_DATA);
    uint8 slaveMask = IO::InByte(PIC2_DATA);

    // start the initialization sequence (cascade mode, expect ICW4)
    IO::OutByte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    IO::OutByte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // set interrupt vector offsets
    IO::OutByte(PIC1_DATA, offset1);
    IO::OutByte(PIC2_DATA, offset2);

    // tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    IO::OutByte(PIC1_DATA, 0x04);

    // tell Slave PIC its cascade identity (0000 0010)
    IO::OutByte(PIC2_DATA, 0x02);

    // set 8086/88 mode
    IO::OutByte(PIC1_DATA, ICW4_8086);
    IO::OutByte(PIC2_DATA, ICW4_8086);

    // restore saved masks
    IO::OutByte(PIC1_DATA, masterMask);
    IO::OutByte(PIC2_DATA, slaveMask);
  }

  void PIC::SendEOI(uint8 irq) {
    if (irq >= 8) {
      IO::OutByte(PIC2_COMMAND, PIC_EOI);
    }

    IO::OutByte(PIC1_COMMAND, PIC_EOI);
  }

  void PIC::Mask(uint8 irq) {
    uint16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) {
      irq -= 8;
    }

    uint8 mask = IO::InByte(port);
    mask |= static_cast<uint8>(1 << irq);
    IO::OutByte(port, mask);
  }

  void PIC::Unmask(uint8 irq) {
    uint16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8) {
      irq -= 8;
    }

    uint8 mask = IO::InByte(port);
    mask &= static_cast<uint8>(~(1 << irq));
    IO::OutByte(port, mask);
  }
}
