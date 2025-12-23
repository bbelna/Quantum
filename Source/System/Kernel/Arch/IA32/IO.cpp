/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/IO.cpp
 * IA32 port I/O primitives.
 */

#include "Arch/IA32/IO.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  UInt8 IO::In8(UInt16 port) {
    UInt8 value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  UInt16 IO::In16(UInt16 port) {
    UInt16 value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  UInt32 IO::In32(UInt16 port) {
    UInt32 value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  void IO::Out8(UInt16 port, UInt8 value) {
    asm volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
  }

  void IO::Out16(UInt16 port, UInt16 value) {
    asm volatile ("outw %0, %1" :: "a"(value), "Nd"(port));
  }

  void IO::Out32(UInt16 port, UInt32 value) {
    asm volatile ("outl %0, %1" :: "a"(value), "Nd"(port));
  }
}
