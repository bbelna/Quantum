/**
 * @file Kernel/Platform/PC/Drivers/Serial/SerialCOMDriver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Serial::SerialCOMDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "SerialCOMDriver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Serial {
  SerialCOMDriver::SerialCOMDriver(Port port)
    : _basePort(static_cast<UInt16>(port))
  {
    SetISAAddress(_device, { _basePort, 4 });
    UInt16 p;
    p = _basePort + 1; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x00)), "Nd"(p)); // disable interrupts
    p = _basePort + 3; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x80)), "Nd"(p)); // enable DLAB
    p = _basePort + 0; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x01)), "Nd"(p)); // divisor low byte (115200)
    p = _basePort + 1; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x00)), "Nd"(p)); // divisor high byte
    p = _basePort + 3; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x03)), "Nd"(p)); // 8 bits, no parity, one stop bit
    p = _basePort + 2; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0xC7)), "Nd"(p)); // enable FIFO, clear, 14-byte threshold
    p = _basePort + 4; asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(0x0B)), "Nd"(p)); // IRQs enabled, RTS/DSR set
  }

  void SerialCOMDriver::Write(char character) {
    if (character == '\n') Write('\r');

    // bounded spin: give up after ~100k iterations to prevent infinite
    // hang if the UART is unresponsive or absent
    UInt32 timeout = 100000;

    while (!_canTransmit() && --timeout) {}

    if (timeout == 0) return;

    UInt16 bp = _basePort;
    asm volatile("outb %0, %1" :: "a"(static_cast<UInt8>(character)), "Nd"(bp));
  }

  void SerialCOMDriver::Write(const char* string) {
    while (*string) Write(*string++);
  }

  bool SerialCOMDriver::_canTransmit() {
    UInt16 lsrPort = _basePort + 5;
    UInt8 val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(lsrPort));
    return (val & 0x20) != 0;
  }
}
