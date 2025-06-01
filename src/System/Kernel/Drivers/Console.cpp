//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// Kernel/Drivers/Console.cpp
// The kernel's console driver.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Console.hpp>

using namespace Kernel::Drivers;

extern "C" inline void outb(uint16  port, uint8  value) {
  asm volatile ("outb %0, %1" :: "a"(value), "Nd"(port));
}

void Console::UpdateCursor() {
  // Scroll now if row has overflowed
  if (row >= 25) {
    // Copy lines 1..24 → 0..23
    for (uint16 r = 1; r < 25; ++r) {
      for (uint16 c = 0; c < 80; ++c) {
        buffer[Index(r - 1, c)] = buffer[Index(r, c)];
      }
    }

    // Clear line 24
    uint16 blank = (uint16) ' ' | ((uint16) defaultColor << 8);
    for (uint16 c = 0; c < 80; ++c) {
      buffer[Index(24, c)] = blank;
    }

    row = 24;
    col = 0;
  }

  uint16 pos = Index(row, col);

  // Cursor register ports: 0x3D4 = index, 0x3D5 = data
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8 )(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8 )((pos >> 8) & 0xFF));
}

void Console::Initialize() {
  uint16 blank = (uint16 )' ' | ((uint16 )defaultColor << 8);
  for (uint16 r = 0; r < 25; ++r) {
    for (uint16 c = 0; c < 80; ++c) {
      buffer[Index(r, c)] = blank;
    }
  }
  row = 0;
  col = 0;
  UpdateCursor();
}

void Console::WriteChar(char c) {
  if (c == '\n') {
    col = 0;
    row++;
  } else if (c == '\r') {
    col = 0;
  } else {
    uint16 attr = (uint16 )defaultColor << 8;
    buffer[Index(row, col)] = ((uint16 )c) | attr;
    col++;
    if (col >= 80) {
      col = 0;
      row++;
    }
  }
  UpdateCursor();
}

void Console::WriteString(const char* str) {
  for (const char* p = str; *p != '\0'; ++p) {
    WriteChar(*p);
  }
}
