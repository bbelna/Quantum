/**
 * @file Bootloader/HAL/IGraphicsDriver.hpp
 * @brief Declares @ref @QBtldr::HAL::IGraphicsDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <HAL/HALTypes.hpp>

namespace Quantum::Bootloader::HAL {
  /**
   * @brief Abstract interface for graphics drivers in the bootloader
   *        environment.
   */
  class IGraphicsDriver {
    public:
      /**
       * @brief Destroys this @ref IGraphicsDriver instance.
       */
      virtual ~IGraphicsDriver() = default;

      /**
       * @brief Sets the text-mode cursor position.
       * @param column The column to set the cursor to (`0`-based).
       * @param row The row to set the cursor to (`0`-based).
       */
      virtual void SetTextCursor(UInt8 column, UInt8 row) = 0;

      /**
       * @brief Prints a single character to the screen at the current cursor
       *        position.
       * @param c The character to print.
       * @param column The column to print the character at (`0`-based).
       * @param row The row to print the character at (`0`-based).
       */
      virtual void Put(char c, UInt8 column, UInt8 row) = 0;

      /**
       * @brief Prints a null-terminated string to the screen at the current
       *        cursor position.
       * @param string The string to print.
       */
      virtual void Print(const char* string) = 0;

      /**
       * @brief Sets the active text foreground and background colors.
       * @param fg Foreground color index (`0`–`15`, standard CGA palette).
       * @param bg Background color index (`0`–`7`, standard CGA palette).
       *
       * Affects all subsequent @ref Print and @ref Put calls.
       */
      virtual void SetTextColor(UInt8 fg, UInt8 bg) = 0;

      /**
       * @brief Makes the cursor visible.
       *
       * Renders a solid full-block cursor at the current cursor position.
       * Has no effect if the cursor is already visible.
       */
      virtual void ShowTextCursor() = 0;

      /**
       * @brief Hides the cursor.
       *
       * Has no effect if the cursor is already hidden.
       */
      virtual void HideTextCursor() = 0;

      /**
       * @brief Returns the current text-mode cursor column.
       * @return The `0`-based column index.
       */
      virtual UInt8 GetTextCursorColumn() const = 0;

      /**
       * @brief Returns the current text-mode cursor row.
       * @return The `0`-based row index.
       */
      virtual UInt8 GetTextCursorRow() const = 0;

      /**
       * @brief Gets the number of text columns supported by this driver.
       * @return The number of text columns.
       */
      virtual UInt8 GetTextColumns() const = 0;

      /**
       * @brief Gets the number of text rows supported by this driver.
       * @return The number of text rows.
       */
      virtual UInt8 GetTextRows() const = 0;

      /**
       * @brief Clears a single text row.
       * @param row The `0`-based row index to clear.
       *
       * Fills every column in the given row with a space using the current
       * color attribute. The cursor position is not changed.
       */
      virtual void ClearRow(UInt8 row) = 0;

      /**
       * @brief Clears the entire screen and resets the cursor to \f$(0,\, 0)\f$.
       *
       * Fills every cell with a space using the current color attribute.
       */
      virtual void ClearScreen() = 0;
  };
}
