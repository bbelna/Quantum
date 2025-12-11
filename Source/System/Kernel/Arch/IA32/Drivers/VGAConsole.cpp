//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Drivers/VGAConsole.cpp
// (c) 2025 Brandon Belna - MIT LIcense
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
    constexpr UInt8 columns = 80;

    /**
     * The number of text-mode rows.
     */
    constexpr UInt8 rows = 25;

    /**
     * The default text color (light gray on black).
     */
    constexpr UInt8 defaultColor = 0x0F;

    /**
     * The VGA text-mode buffer.
     */
    volatile UInt16* const buffer
      = reinterpret_cast<volatile UInt16*>(0xB8000);

    /**
     * The current cursor row.
     */
    UInt8 cursorRow = 0;

    /**
     * The current cursor column.
     */
    UInt8 cursorColumn = 0;

    /**
     * The saved cell value under the cursor.
     */
    UInt16 cursorSavedCell = 0;

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
    inline UInt16 Index(UInt8 row, UInt8 column) {
      return static_cast<UInt16>(row * columns + column);
    }

    /**
     * Creates a VGA text-mode entry from a character and color.
     * @param character The character.
     * @param color The color attribute.
     * @return The VGA text-mode entry.
     */
    inline UInt16 MakeEntry(char character, UInt8 color) {
      return static_cast<UInt16>(character) | (static_cast<UInt16>(color) << 8);
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
      UInt8 blockAttr = 0xFF;
      UInt16 blockCell = MakeEntry(' ', blockAttr);

      buffer[Index(cursorRow, cursorColumn)] = blockCell;
      cursorDrawn = true;
    }

  }

  void VGAConsole::Initialize() {
    UInt16 blank = MakeEntry(' ', defaultColor);

    for (UInt16 r = 0; r < rows; ++r) {
      for (UInt16 c = 0; c < columns; ++c) {
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

  void VGAConsole::WriteCharacter(char character) {
    HideCursor();

    switch (character) {
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
        UInt16 entry = MakeEntry(character, defaultColor);
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
      for (UInt16 r = 1; r < rows; ++r) {
        for (UInt16 c = 0; c < columns; ++c) {
          buffer[Index(r - 1, c)] = buffer[Index(r, c)];
        }
      }

      UInt16 blank = MakeEntry(' ', defaultColor);
      for (UInt16 c = 0; c < columns; ++c) {
        buffer[Index(rows - 1, c)] = blank;
      }

      cursorRow = rows - 1;
      cursorColumn = 0;
    }

    DrawCursor();
  }

  void VGAConsole::Write(CString message) {
    for (
      CString messageIterator = message;
      *messageIterator != '\0';
      ++messageIterator
    ) {
      WriteCharacter(*messageIterator);
    }
  }

  void VGAConsole::WriteLine(CString message) {
    Write(message);
    WriteCharacter('\n');
  }

  Writer& VGAConsole::GetWriter() {
    static WriterAdapter writerAdapter;
    return writerAdapter;
  }

  void VGAConsole::WriterAdapter::Write(String message) {
    VGAConsole::WriteLine(message);
  }
}
