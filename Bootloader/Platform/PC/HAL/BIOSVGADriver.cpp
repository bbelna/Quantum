/**
 * @file Bootloader/Platform/PC/HAL/BIOSVGADriver.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::HAL::BIOSVGADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "BIOSVGADriver.hpp"

namespace Quantum::Bootloader::Platform::PC::HAL {
  BIOSVGADriver::BIOSVGADriver() {
    BIOSRegisters regs = {};

    regs.EAX = 0x0003; // AH=00h: set video mode, AL=03h: 80x25 color text

    CallInterrupt(0x10, regs);

    // Hide the hardware cursor, a solid block cursor is shown on demand
    // via ShowCursor(). CH bit 5 set = cursor disabled.
    regs = {};
    regs.EAX = 0x0100; // AH=01h: set cursor shape
    regs.ECX = 0x2000; // CH=0x20 (bit 5 set = hide), CL=0x00

    CallInterrupt(0x10, regs);
  }

  void BIOSVGADriver::SetTextCursor(UInt8 column, UInt8 row) {
    if (column >= Columns) column = Columns - 1;
    if (row >= Rows) row = Rows - 1;

    if (_cursorVisible) _writeVGACell(_row, _column, ' ', _color);

    _column = column;
    _row = row;

    _syncCursor();

    if (_cursorVisible) _writeVGACell(_row, _column, 0xDB, 0x0F);
  }

  void BIOSVGADriver::Put(char character, UInt8 column, UInt8 row) {
    SetTextCursor(column, row);
    _put(character);
  }

  void BIOSVGADriver::Print(const char* string) {
    while (*string) _put(*string++);
  }

  void BIOSVGADriver::SetTextColor(UInt8 fg, UInt8 bg) {
    _color = (static_cast<UInt8>(bg & 0x0F) << 4) | (fg & 0x0F);
  }

  void BIOSVGADriver::ShowTextCursor() {
    _cursorVisible = true;

    // Write CP437 0xDB (█) with bright-white-on-black directly into the VGA
    // text buffer, solid, non-blinking, no hardware cursor involved.
    _writeVGACell(_row, _column, 0xDB, 0x0F);
  }

  void BIOSVGADriver::HideTextCursor() {
    if (!_cursorVisible) return;

    _cursorVisible = false;

    _writeVGACell(_row, _column, ' ', _color);
  }

  UInt8 BIOSVGADriver::GetTextCursorColumn() const { return _column; }

  UInt8 BIOSVGADriver::GetTextCursorRow() const { return _row; }

  void BIOSVGADriver::ClearRow(UInt8 row) {
    if (row >= Rows) row = Rows - 1;

    for (UInt8 col = 0; col < Columns; col++) {
      _writeVGACell(row, col, ' ', _color);
    }

    // Redraw the software cursor if it lives on this row.
    if (_cursorVisible && _row == row) {
      _writeVGACell(_row, _column, 0xDB, 0x0F);
    }
  }

  void BIOSVGADriver::ClearScreen() {
    for (UInt8 row = 0; row < Rows; row++) {
      for (UInt8 col = 0; col < Columns; col++) {
        _writeVGACell(row, col, ' ', _color);
      }
    }

    _column = 0;
    _row = 0;

    _syncCursor();

    if (_cursorVisible) _writeVGACell(_row, _column, 0xDB, 0x0F);
  }

  void BIOSVGADriver::_writeVGACell(UInt8 row, UInt8 col, UInt8 ch, UInt8 attr) {
    auto* vga = reinterpret_cast<volatile UInt16*>(0xB8000);

    vga[row * Columns + col] =
      (static_cast<UInt16>(attr) << 8) | static_cast<UInt16>(ch);
  }

  void BIOSVGADriver::_put(char c) {
    // Erase the software cursor before mutating the cursor position or
    // writing a character, it will be redrawn at the new position below.
    if (_cursorVisible) _writeVGACell(_row, _column, ' ', _color);

    BIOSRegisters regs = {};

    if (c == '\r') {
      _column = 0;
      _syncCursor();
    } else if (c == '\n') {
      _column = 0;
      _advanceLine();
    } else if (c == '\b') {
      if (_column > 0) {
        _column--;
        _syncCursor();
      }
    } else {
      // Write character with color attribute at the current cursor position.
      // AH=09h does not advance the cursor, so we do it manually below.
      regs.EAX = 0x0900 | static_cast<UInt8>(c); // AH=09h, AL=char
      regs.EBX = static_cast<UInt32>(_color);     // BH=page 0, BL=attribute
      regs.ECX = 1;                               // CX=repeat count

      CallInterrupt(0x10, regs);

      _column++;

      if (_column >= Columns) {
        _column = 0;
        _advanceLine();
      } else {
        _syncCursor();
      }
    }

    // Redraw the cursor at the new position.
    if (_cursorVisible) _writeVGACell(_row, _column, 0xDB, 0x0F);
  }

  void BIOSVGADriver::_syncCursor() {
    BIOSRegisters regs = {};

    regs.EAX = 0x0200;                                    // AH=02h: set cursor position
    regs.EBX = 0x0000;                                    // BH=00h: page 0
    regs.EDX = (static_cast<UInt32>(_row) << 8) | _column;

    CallInterrupt(0x10, regs);
  }

  void BIOSVGADriver::_advanceLine() {
    if (_row < Rows - 1) {
      _row++;
      _syncCursor();

      return;
    }

    // Last row reached, scroll the entire viewport up one line.
    // AH=06h: scroll up, AL=1 line, BH=fill attribute, CX=top-left, DX=bottom-right.
    BIOSRegisters regs = {};

    regs.EAX = 0x0601;                                              // AH=06h, AL=1
    regs.EBX = static_cast<UInt32>(_color) << 8;                   // BH=fill attribute
    regs.ECX = 0x0000;                                              // CH=0, CL=0 (top-left)
    regs.EDX = (static_cast<UInt32>(Rows - 1) << 8) | (Columns - 1); // bottom-right

    CallInterrupt(0x10, regs);

    // Cursor stays on the last row; _col was already reset by the caller.
    _syncCursor();
  }
}
