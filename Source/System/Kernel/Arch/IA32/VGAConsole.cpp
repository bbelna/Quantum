/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/VGAConsole.cpp
 * IA32 VGA text-mode console driver.
 */

#include <Arch/IA32/IO.hpp>
#include <Arch/IA32/VGAConsole.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using Writer = Logger::Writer;

  namespace {
    /**
     * The number of text-mode columns.
     */
    constexpr UInt8 _columns = 80;

    /**
     * The number of text-mode rows.
     */
    constexpr UInt8 _rows = 25;

    /**
     * The default text color (light gray on black).
     */
    constexpr UInt8 _defaultColor = 0x0F;

    /**
     * The VGA text-mode buffer.
     */
    volatile UInt16* const _buffer
      = reinterpret_cast<volatile UInt16*>(0xB8000);

    /**
     * The current cursor row.
     */
    UInt8 _cursorRow = 0;

    /**
     * The current cursor column.
     */
    UInt8 _cursorColumn = 0;

    /**
     * The saved cell value under the cursor.
     */
    UInt16 _cursorSavedCell = 0;

    /**
     * Whether the cursor is currently drawn.
     */
    bool _cursorDrawn = false;

    /**
     * Calculates the linear index in the VGA buffer for the given row and
     * column.
     * @param row
     *   The row.
     * @param column
     *   The column.
     * @return
     *   The linear index.
     */
    inline UInt16 Index(UInt8 row, UInt8 column) {
      return static_cast<UInt16>(row * _columns + column);
    }

    /**
     * Creates a VGA text-mode entry from a character and color.
     * @param character
     *   The character.
     * @param color
     *   The color attribute.
     * @return
     *   The VGA text-mode entry.
     */
    inline UInt16 MakeEntry(char character, UInt8 color) {
      return static_cast<UInt16>(character) | (static_cast<UInt16>(color) << 8);
    }

    /**
     * Hides the cursor.
     */
    void HideCursor() {
      if (_cursorDrawn) {
        _buffer[Index(_cursorRow, _cursorColumn)] = _cursorSavedCell;
        _cursorDrawn = false;
      }
    }

    /**
     * Draws the cursor.
     */
    void DrawCursor() {
      _cursorSavedCell = _buffer[Index(_cursorRow, _cursorColumn)];

      // solid block: fg=white, bg=white (with blink bit set; fine for now)
      UInt8 blockAttr = 0xFF;
      UInt16 blockCell = MakeEntry(' ', blockAttr);

      _buffer[Index(_cursorRow, _cursorColumn)] = blockCell;
      _cursorDrawn = true;
    }

  }

  void VGAConsole::Initialize() {
    UInt16 blank = MakeEntry(' ', _defaultColor);

    for (UInt16 r = 0; r < _rows; ++r) {
      for (UInt16 c = 0; c < _columns; ++c) {
        _buffer[Index(r, c)] = blank;
      }
    }

    _cursorRow = 0;
    _cursorColumn = 0;

    // hide the hardware cursor
    IO::Out8(0x3D4, 0x0A);
    IO::Out8(0x3D5, 0x20);

    DrawCursor();
  }

  void VGAConsole::WriteCharacter(char character) {
    HideCursor();

    switch (character) {
      case '\n':
        _cursorColumn = 0;
        _cursorRow++;
        break;
      case '\r':
        _cursorColumn = 0;
        break;
      case '\b':
        if (_cursorColumn > 0) {
          _cursorColumn--;
          _buffer[Index(_cursorRow, _cursorColumn)]
            = MakeEntry(' ', _defaultColor);
        } else if (_cursorRow > 0) {
          _cursorRow--;
          _cursorColumn = 79;
          _buffer[Index(_cursorRow, _cursorColumn)]
            = MakeEntry(' ', _defaultColor);
        }
        break;
      default: {
        UInt16 entry = MakeEntry(character, _defaultColor);
        _buffer[Index(_cursorRow, _cursorColumn)] = entry;
        _cursorColumn++;
        if (_cursorColumn >= _columns) {
          _cursorColumn = 0;
          _cursorRow++;
        }
        break;
      }
    }

    if (_cursorRow >= _rows) {
      for (UInt16 r = 1; r < _rows; ++r) {
        for (UInt16 c = 0; c < _columns; ++c) {
          _buffer[Index(r - 1, c)] = _buffer[Index(r, c)];
        }
      }

      UInt16 blank = MakeEntry(' ', _defaultColor);
      for (UInt16 c = 0; c < _columns; ++c) {
        _buffer[Index(_rows - 1, c)] = blank;
      }

      _cursorRow = _rows - 1;
      _cursorColumn = 0;
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
