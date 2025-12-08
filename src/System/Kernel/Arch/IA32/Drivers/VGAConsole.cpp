//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/VGAConsole.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 kernel VGA console driver.
//------------------------------------------------------------------------------

#include <Arch/IA32/Drivers/IO.hpp>
#include <Arch/IA32/Drivers/VGAConsole.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  namespace IO = Quantum::Kernel::Arch::IA32::Drivers::IO;

  volatile uint16* const VGAConsole::buffer
    = reinterpret_cast<volatile uint16*>(0xB8000);

  // TODO? VGAConsoleCursorState class?
  uint8 VGAConsole::cursorRow = 0;
  uint8 VGAConsole::cursorColumn = 0;
  uint16 VGAConsole::cursorSavedCell = 0;
  bool VGAConsole::cursorDrawn = false;

  void VGAConsole::Initialize() {
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

  void VGAConsole::WriteChar(char c) {
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

  void VGAConsole::Write(const char* str) {
    for (const char* p = str; *p != '\0'; ++p) {
      WriteChar(*p);
    }
  }

  void VGAConsole::WriteLine(const char* str) {
    Write(str);
    WriteChar('\n');
  }

  void VGAConsole::HideCursor() {
    if (cursorDrawn) {
      buffer[Index(cursorRow, cursorColumn)] = cursorSavedCell;
      cursorDrawn = false;
    }
  }

  void VGAConsole::DrawCursor() {
    cursorSavedCell = buffer[Index(cursorRow, cursorColumn)];

    // Solid block: fg=white, bg=white (with blink bit set; fine for now)
    uint8 blockAttr = 0xFF;
    uint16 blockCell = MakeEntry(' ', blockAttr);

    buffer[Index(cursorRow, cursorColumn)] = blockCell;
    cursorDrawn = true;
  }
}