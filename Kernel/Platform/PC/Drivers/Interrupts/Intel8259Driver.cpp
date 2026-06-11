/**
 * @file Kernel/Platform/PC/Drivers/Interrupts/Intel8259Driver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Interrupts::Intel8259Driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Drivers/CPU/ICPUDriver.hpp>
#include <KernelLog.hpp>

#include "Intel8259Driver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Interrupts {
  Intel8259Driver::Intel8259Driver(
    ICPUDriver& cpu,
    UInt32 pic1Offset,
    UInt32 pic2Offset
  ) : _cpu(cpu), _pic1Offset(pic1Offset), _pic2Offset(pic2Offset) {
    SetISAAddress(_device, { PIC1_COMMAND_PORT, 0 });

    // preserve current masks so we restore them after the remap
    UInt8 pic1Mask = _cpu.In8(PIC1_DATA_PORT);
    UInt8 pic2Mask = _cpu.In8(PIC2_DATA_PORT);

    // start the initialization sequence (cascade mode, expect ICW4)
    _cpu.Out8(PIC1_COMMAND_PORT, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    _cpu.Out8(PIC2_COMMAND_PORT, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    // set interrupt vector offsets
    _cpu.Out8(PIC1_DATA_PORT, pic1Offset);
    _cpu.Out8(PIC2_DATA_PORT, pic2Offset);

    // tell PIC 1 there is a PIC 2 at IRQ2 (0000 0100)
    _cpu.Out8(PIC1_DATA_PORT, 0x04);

    // tell PIC 2 its cascade identity (0000 0010)
    _cpu.Out8(PIC2_DATA_PORT, 0x02);

    // set 8086/88 mode
    _cpu.Out8(PIC1_DATA_PORT, PIC_ICW4_8086);
    _cpu.Out8(PIC2_DATA_PORT, PIC_ICW4_8086);

    // restore saved masks
    _cpu.Out8(PIC1_DATA_PORT, pic1Mask);
    _cpu.Out8(PIC2_DATA_PORT, pic2Mask);

    KLOG_TRACE(
      "Intel 8259 kernel driver initialized with offsets %u and %u",
      pic1Offset,
      pic2Offset
    );
  }

  void Intel8259Driver::End(UInt8 vector) {
    // if the interrupt came from PIC 2, we need to send an EOI to it first
    if (vector >= 8) _cpu.Out8(PIC2_COMMAND_PORT, PIC_EOI);

    // send EOI to PIC 1
    _cpu.Out8(PIC1_COMMAND_PORT, PIC_EOI);
  }

  void Intel8259Driver::Mask(UInt8 vector) {
    // determine which PIC the vector belongs to
    UInt16 port = (vector < 8) ? PIC1_DATA_PORT : PIC2_DATA_PORT;

    // adjust vector for PIC 2
    if (vector >= 8) vector -= 8;

    // read-modify-write the mask
    UInt8 mask = _cpu.In8(port);

    // set the bit to mask the interrupt
    mask |= static_cast<UInt8>(1 << vector);

    // write back the new mask
    _cpu.Out8(port, mask);
  }

  void Intel8259Driver::MaskAll() {
    // set all bits in both PIC masks
    _cpu.Out8(PIC1_DATA_PORT, 0xFF);
    _cpu.Out8(PIC2_DATA_PORT, 0xFF);
  }

  void Intel8259Driver::Unmask(UInt8 vector) {
    // determine which PIC the vector belongs to
    UInt16 port = (vector < 8) ? PIC1_DATA_PORT : PIC2_DATA_PORT;

    // adjust vector for PIC 2
    if (vector >= 8) vector -= 8;

    // read-modify-write the mask
    UInt8 mask = _cpu.In8(port);

    // clear the bit to unmask the interrupt
    mask &= static_cast<UInt8>(~(1 << vector));

    // write back the new mask
    _cpu.Out8(port, mask);
  }

  void Intel8259Driver::UnmaskAll() {
    // clear all bits in both PIC masks
    _cpu.Out8(PIC1_DATA_PORT, 0x00);
    _cpu.Out8(PIC2_DATA_PORT, 0x00);
  }
}
