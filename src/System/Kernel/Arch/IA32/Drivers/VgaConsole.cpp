//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Arch/IA32/Drivers/VgaConsole.cpp
// IA32 kernel VGA console driver.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/VgaConsole.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace IO = Quantum::Kernel::Arch::IA32::Drivers::IO;

  volatile uint16* const VgaConsole::buffer
    = reinterpret_cast<volatile uint16*>(0xB8000);

  uint8 VgaConsole::cursorRow = 0;
  uint8 VgaConsole::cursorColumn = 0;
  uint16 VgaConsole::cursorSavedCell = 0;
  bool VgaConsole::cursorDrawn = false;

  void VgaConsole::Initialize() {
    uint16 blank = (uint16)' ' | ((uint16)defaultColor << 8);

    for (uint16 r = 0; r < 25; ++r) {
      for (uint16 c = 0; c < 80; ++c) {
        buffer[Index(r, c)] = blank;
      }
    }

    cursorRow = 0;
    cursorColumn = 0;

    // hide the hardware cursor
    IO::OutByte(0x3D4, 0x0A);
    IO::OutByte(0x3D5, 0x20);

    DrawCursor();
  }

  void VgaConsole::WriteChar(char c) {
    HideCursor();

    if (c == '\n') {
      cursorColumn = 0;
      cursorRow++;
    } else if (c == '\r') {
      cursorColumn = 0;
    } else {
      uint16 attr = (uint16)defaultColor << 8;
      buffer[Index(cursorRow, cursorColumn)] = ((uint16)c) | attr;
      cursorColumn++;
      if (cursorColumn >= 80) {
        cursorColumn = 0;
        cursorRow++;
      }
    }

    if (cursorRow >= 25) {
      for (uint16 r = 1; r < 25; ++r) {
        for (uint16 c = 0; c < 80; ++c) {
          buffer[Index(r - 1, c)] = buffer[Index(r, c)];
        }
      }

      uint16 blank = (uint16)' ' | ((uint16)defaultColor << 8);
      for (uint16 c = 0; c < 80; ++c) {
        buffer[Index(24, c)] = blank;
      }

      cursorRow = 24;
      cursorColumn = 0;
    }

    DrawCursor();
  }

  void VgaConsole::WriteString(const char* str) {
    for (const char* p = str; *p != '\0'; ++p) {
      WriteChar(*p);
    }
  }

  void VgaConsole::HideCursor() {
    if (cursorDrawn) {
      buffer[Index(cursorRow, cursorColumn)] = cursorSavedCell;
      cursorDrawn = false;
    }
  }

  void VgaConsole::DrawCursor() {
    cursorSavedCell = buffer[Index(cursorRow, cursorColumn)];

    // Solid block: fg=white, bg=white (with blink bit set; fine for now)
    uint8 blockAttr = 0xFF;
    uint16 blockCell = MakeEntry(' ', blockAttr);

    buffer[Index(cursorRow, cursorColumn)] = blockCell;
    cursorDrawn = true;
}
}