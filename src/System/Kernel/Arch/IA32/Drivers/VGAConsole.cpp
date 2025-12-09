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
  namespace {
    /**
     * The number of text-mode columns.
     */
    constexpr uint8 columns = 80;

    /**
     * The number of text-mode rows.
     */
    constexpr uint8 rows = 25;

    /**
     * The default text color (light gray on black).
     */
    constexpr uint8 defaultColor = 0x0F;

    /**
     * The VGA text-mode buffer.
     */
    volatile uint16* const buffer
      = reinterpret_cast<volatile uint16*>(0xB8000);

    /**
     * The current cursor row.
     */
    uint8 cursorRow = 0;

    /**
     * The current cursor column.
     */
    uint8 cursorColumn = 0;

    /**
     * The saved cell value under the cursor.
     */
    uint16 cursorSavedCell = 0;

    /**
     * Whether the cursor is currently drawn.
     */
    bool cursorDrawn = false;

    /**
     * Calculates the linear index in the VGA buffer for the given row and
     * column.
     * @param row The row.
     * @param column The column.
     * @return The linear index.
     */
    inline uint16 Index(uint8 row, uint8 column) {
      return static_cast<uint16>(row * columns + column);
    }

    /**
     * Creates a VGA text-mode entry from a character and color.
     * @param character The character.
     * @param color The color attribute.
     * @return The VGA text-mode entry.
     */
    inline uint16 MakeEntry(char character, uint8 color) {
      return static_cast<uint16>(character) | (static_cast<uint16>(color) << 8);
    }

    /**
     * Hides the cursor.
     */
    void HideCursor() {
      if (cursorDrawn) {
        buffer[Index(cursorRow, cursorColumn)] = cursorSavedCell;
        cursorDrawn = false;
      }
    }

    /**
     * Draws the cursor.
     */
    void DrawCursor() {
      cursorSavedCell = buffer[Index(cursorRow, cursorColumn)];

      // solid block: fg=white, bg=white (with blink bit set; fine for now)
      uint8 blockAttr = 0xFF;
      uint16 blockCell = MakeEntry(' ', blockAttr);

      buffer[Index(cursorRow, cursorColumn)] = blockCell;
      cursorDrawn = true;
    }
  }

  void VGAConsole::Initialize() {
    uint16 blank = MakeEntry(' ', defaultColor);

    for (uint16 r = 0; r < rows; ++r) {
      for (uint16 c = 0; c < columns; ++c) {
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

    switch (c) {
      case '\n':
        cursorColumn = 0;
        cursorRow++;
        break;
      case '\r':
        cursorColumn = 0;
        break;
      case '\b':
        if (cursorColumn > 0) {
          cursorColumn--;
          buffer[Index(cursorRow, cursorColumn)] = MakeEntry(' ', defaultColor);
        } else if (cursorRow > 0) {
          cursorRow--;
          cursorColumn = 79;
          buffer[Index(cursorRow, cursorColumn)] = MakeEntry(' ', defaultColor);
        }
        break;
      default: {
        uint16 entry = MakeEntry(c, defaultColor);
        buffer[Index(cursorRow, cursorColumn)] = entry;
        cursorColumn++;
        if (cursorColumn >= columns) {
          cursorColumn = 0;
          cursorRow++;
        }
        break;
      }
    }

    if (cursorRow >= rows) {
      for (uint16 r = 1; r < rows; ++r) {
        for (uint16 c = 0; c < columns; ++c) {
          buffer[Index(r - 1, c)] = buffer[Index(r, c)];
        }
      }

      uint16 blank = MakeEntry(' ', defaultColor);
      for (uint16 c = 0; c < columns; ++c) {
        buffer[Index(rows - 1, c)] = blank;
      }

      cursorRow = rows - 1;
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

  void VGAConsole::WriteHex32(uint32 value) {
    const char* hex = "0123456789ABCDEF";
    Write("0x");

    for (int shift = 28; shift >= 0; shift -= 4) {
      uint8 nibble = static_cast<uint8>((value >> shift) & 0xF);
      WriteChar(hex[nibble]);
    }
  }
}
