/**
 * @file Bootloader/Platform/PC/HAL/BIOSVGADriver.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::HAL::BIOSVGADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "BIOSDriver.hpp"

namespace Quantum::Bootloader::Platform::PC::HAL {
  /**
   * @brief BIOS-based VGA text-mode graphics driver for the bootloader
   *        environment.
   *
   * Implements @ref IGraphicsDriver using BIOS `INT 10h` calls. The
   * constructor switches to \f$80 \times 25\f$ color text mode (mode `03h`),
   * clears the screen, resets the cursor to \f$(0,\, 0)\f$, and hides the
   * hardware cursor. The cursor remains hidden until @ref ShowCursor is called.
   */
  class BIOSVGADriver : public BIOSDriver, public IGraphicsDriver {
    public:
      /**
       * @brief The number of text columns in the current mode.
       */
      constexpr static UInt8 Columns = 80;

      /**
       * @brief The number of text rows in the current mode.
       */
      constexpr static UInt8 Rows = 25;

      /**
       * @brief Constructs a @ref BIOSVGADriver and initializes
       *        \f$80 \times 25\f$ color text mode.
       *
       * Sets video mode 03h, then disables the hardware cursor via
       * `INT 10h` / `AH=01h` with `CH` bit 5 set. The cursor is not
       * visible until @ref ShowCursor is called.
       */
      BIOSVGADriver();

      /**
       * @brief Sets the text-mode cursor position.
       * @param column Column to move to (`0`-based, clamped to `79`).
       * @param row Row to move to (`0`-based, clamped to `24`).
       */
      void SetTextCursor(UInt8 column, UInt8 row) override;

      /**
       * @brief Prints a single character at a specific text-mode position.
       * @param character The character to print.
       * @param column Column to print at (`0`-based).
       * @param row Row to print at (`0`-based).
       */
      void Put(char character, UInt8 column, UInt8 row) override;

      /**
       * @brief Prints a null-terminated string at the current cursor position.
       * @param string The string to print. `\\n` is expanded to `CR+LF`.
       *
       * Characters are written with the current color attribute set by
       * @ref SetColor. Scrolling is handled automatically when the last row
       * is reached.
       */
      void Print(const char* string) override;

      /**
       * @brief Sets the active foreground and background colors.
       * @param fg Foreground color index (`0`–`15`, CGA palette).
       * @param bg Background color index (`0`–`7`, CGA palette).
       *
       * Affects all subsequent @ref Print and @ref Put calls.
       */
      void SetTextColor(UInt8 fg, UInt8 bg) override;

      /**
       * @brief Makes the cursor visible as a solid white full-block glyph.
       *
       * Writes CP437 `0xDB` with a bright-white-on-black attribute
       * directly into the VGA text buffer at the current cursor position.
       */
      void ShowTextCursor() override;

      /**
       * @brief Hides the cursor by erasing the block glyph.
       *
       * Overwrites the cell at the current cursor position with a space
       * using the active color attribute. Has no effect if the cursor is
       * already hidden.
       */
      void HideTextCursor() override;

      /**
       * @brief Returns the current text-mode cursor column.
       * @return The `0`-based column index.
       */
      UInt8 GetTextCursorColumn() const override;

      /**
       * @brief Returns the current text-mode cursor row.
       * @return The `0`-based row index.
       */
      UInt8 GetTextCursorRow() const override;

      /**
       * @brief Gets the number of text columns supported by this driver.
       * @return The number of text columns.
       */
      UInt8 GetTextColumns() const override {
        return Columns;
      }

      /**
       * @brief Gets the number of text rows supported by this driver.
       * @return The number of text rows.
       */
      UInt8 GetTextRows() const override {
        return Rows;
      }

      /**
       * @brief Clears a single text row.
       * @param row The `0`-based row index to clear (clamped to `24`).
       *
       * Fills every column in the given row with a space using the current
       * color attribute. The cursor position is not changed.
       */
      void ClearRow(UInt8 row) override;

      /**
       * @brief Clears the entire screen and resets the cursor to
       *        \f$(0,\, 0)\f$.
       *
       * Fills every cell in the \f$80 \times 25\f$ text buffer with a
       * space using the current color attribute, then moves the cursor to
       * the top-left corner.
       */
      void ClearScreen() override;

    private:
      /**
       * @brief Current text-mode column (`0`-based).
       */
      UInt8 _column = 0;

      /**
       * @brief Current text-mode row (`0`-based).
       */
      UInt8 _row = 0;

      /**
       * @brief Active color attribute byte (`bg[7:4] | fg[3:0]`).
       */
      UInt8 _color = 0x0F;

      /**
       * @brief Whether the software block cursor is currently visible.
       */
      bool _cursorVisible = false;

      /**
       * @brief Writes a single cell directly into the VGA text buffer.
       * @param row Target row (`0`-based).
       * @param col Target column (`0`-based).
       * @param ch CP437 character byte.
       * @param attr VGA attribute byte (`bg[7:4] | fg[3:0]`).
       *
       * Writes to physical address `0xB8000 + (row * 80 + col) * 2`
       * without going through the BIOS trampoline.
       */
      static void _writeVGACell(UInt8 row, UInt8 col, UInt8 ch, UInt8 attr);

      /**
       * @brief Writes a printable character at the current position with
       *        the current color attribute, then advances the cursor.
       * @param c The character to write.
       *
       * Uses `INT 10h` / `AH=09h` (write character and attribute) at the
       * tracked cursor position, then calls @ref _syncCursor to move the
       * BIOS cursor forward. Scrolls via `AH=06h` when the last row is
       * exceeded.
       */
      void _put(char c);

      /**
       * @brief Pushes the tracked `(_column, _row)` position to the BIOS
       *        hardware cursor via `INT 10h` / `AH=02h`.
       *
       * Required so `AH=09h` (write character and attribute) writes at the
       * correct cell. The hardware cursor itself is kept hidden.
       */
      void _syncCursor();

      /**
       * @brief Advances to the next line, scrolling the screen if necessary.
       *
       * Increments `_row`. If the screen is full, issues `INT 10h` / `AH=06h`
       * to scroll up one line and keeps the cursor on the last row.
       */
      void _advanceLine();
  };
}
