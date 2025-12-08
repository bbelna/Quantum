//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/IO.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 port I/O primitives.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers::IO {
  inline void OutByte(uint16 port, uint8 value) {
    asm volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
  }

  inline uint8 InByte(uint16 port) {
    uint8 value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  inline void OutWord(uint16 port, uint16 value) {
    asm volatile ("outw %0, %1" :: "a"(value), "Nd"(port));
  }

  inline uint16 InWord(uint16 port) {
    uint16 value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }

  inline void OutDword(uint16 port, uint32 value) {
    asm volatile ("outl %0, %1" :: "a"(value), "Nd"(port));
  }

  inline uint32 InDword(uint16 port) {
    uint32 value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
  }
}
