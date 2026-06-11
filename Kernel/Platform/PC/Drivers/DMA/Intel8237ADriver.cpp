/**
 * @file Kernel/Platform/PC/Drivers/DMA/Intel8237ADriver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::DMA::Intel8237ADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "Intel8237ADriver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::DMA {
  Intel8237ADriver::Intel8237ADriver() { _probe(); }

  void Intel8237ADriver::_probe() {
    bool dma1Present = false;
    bool dma2Present = false;

    // mask DMA channel 1 to prevent accidental transfers during probe
    // bit 2 = set mask, bits [1:0] = channel 1
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x05)), "Nd"(DMA1Mask));

    // reset the flip-flop so the next access hits the low byte
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x00)), "Nd"(DMA1FlipFlop));

    // write a known two-byte value to channel 1 address register
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x34)), "Nd"(DMA1Channel1Address));
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x12)), "Nd"(DMA1Channel1Address));

    // reset the flip-flop and read back the value
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x00)), "Nd"(DMA1FlipFlop));

    UInt8 lowByte;
    asm volatile("inb %1, %0" : "=a"(lowByte) : "Nd"(DMA1Channel1Address));
    UInt8 highByte;
    asm volatile("inb %1, %0" : "=a"(highByte) : "Nd"(DMA1Channel1Address));

    if (lowByte == 0x34 && highByte == 0x12) dma1Present = true;

    // unmask channel 1
    // bit 2 = clear mask, bits [1:0] = channel 1
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x01)), "Nd"(DMA1Mask));

    // verify DMA2 by resetting its flip-flop
    // if the write does not fault and the port is responsive, DMA2 is present
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x00)), "Nd"(DMA2FlipFlop));

    // DMA2 channel 5 address register (port 0xC4) - equivalent probe
    constexpr UInt16 dma2Channel5Address = 0xC4;

    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x34)), "Nd"(dma2Channel5Address));
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x12)), "Nd"(dma2Channel5Address));
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x00)), "Nd"(DMA2FlipFlop));

    UInt8 dma2LowByte;
    asm volatile("inb %1, %0" : "=a"(dma2LowByte) : "Nd"(dma2Channel5Address));
    UInt8 dma2HighByte;
    asm volatile("inb %1, %0" : "=a"(dma2HighByte) : "Nd"(dma2Channel5Address));

    if (dma2LowByte == 0x34 && dma2HighByte == 0x12) dma2Present = true;

    _present = dma1Present && dma2Present;

    if (_present) {
      _device.State = DeviceState::Active;

      KLOG_TRACE("Dual 8237A controllers detected");
    } else {
      _device.State = DeviceState::Error;

      KLOG_WARNING(
        "Probe failed (DMA1 %s, DMA2 %s)",
        dma1Present ? "ok" : "missing",
        dma2Present ? "ok" : "missing"
      );
    }
  }

  UInt32 Intel8237ADriver::Invoke(UInt32 operation, void* payload) {
    (void)operation;
    (void)payload;

    return 0;
  }
}
