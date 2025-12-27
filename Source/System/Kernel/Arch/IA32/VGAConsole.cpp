/**
 * @file System/Kernel/Arch/IA32/VGAConsole.cpp
 * @brief IA32 VGA text-mode console driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/VGAConsole.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  UInt16 VGAConsole::Index(UInt8 row, UInt8 column) {
    return static_cast<UInt16>(row * _columns + column);
  }

  UInt16 VGAConsole::MakeEntry(char character, UInt8 color) {
    return static_cast<UInt16>(character) | (static_cast<UInt16>(color) << 8);
  }

  void VGAConsole::HideCursor() {
    if (_cursorDrawn) {
      _buffer[Index(_cursorRow, _cursorColumn)] = _cursorSavedCell;
      _cursorDrawn = false;
    }
  }

  void VGAConsole::DrawCursor() {
    _cursorSavedCell = _buffer[Index(_cursorRow, _cursorColumn)];

    // solid block: fg=white, bg=white (with blink bit set; fine for now)
    UInt8 blockAttr = 0xFF;
    UInt16 blockCell = MakeEntry(' ', blockAttr);

    _buffer[Index(_cursorRow, _cursorColumn)] = blockCell;
    _cursorDrawn = true;
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
      case '\n': {
        _cursorColumn = 0;
        _cursorRow++;

        break;
      }

      case '\r': {
        _cursorColumn = 0;

        break;
      }

      case '\b': {
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
      }

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
      _cursorDrawn = false;
      _cursorSavedCell = MakeEntry(' ', _defaultColor);
    }

    DrawCursor();
  }
}
