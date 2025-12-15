//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/IO.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// IA32 port I/O primitives.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers::IO {
  /**
   * Outputs a byte to the specified port.
   * @param port
   *   The I/O port.
   * @param value
   *   The byte value to output.
   */
  inline void OutByte(UInt16 port, UInt8 value) {
    asm volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
  }

  /**
   * Inputs a byte from the specified port.
   * @param port
   *   The I/O port.
   * @return
   *   The byte value read from the port.
   */
  inline UInt8 InByte(UInt16 port) {
    UInt8 value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  /**
   * Outputs a word to the specified port.
   * @param port
   *   The I/O port.
   */
  inline void OutWord(UInt16 port, UInt16 value) {
    asm volatile ("outw %0, %1" :: "a"(value), "Nd"(port));
  }

  /**
   * Inputs a word from the specified port.
   * @param port
   *   The I/O port.
   * @return
   *   The word value read from the port.
   */
  inline UInt16 InWord(UInt16 port) {
    UInt16 value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  /**
   * Outputs a double word to the specified port.
   * @param port
   *   The I/O port.
   * @param value
   *   The double word value to output.
   */
  inline void OutDword(UInt16 port, UInt32 value) {
    asm volatile ("outl %0, %1" :: "a"(value), "Nd"(port));
  }

  /**
   * Inputs a double word from the specified port.
   * @param port
   *   The I/O port.
   * @return
   *   The double word value read from the port.
   */
  inline UInt32 InDword(UInt16 port) {
    UInt32 value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }
}
