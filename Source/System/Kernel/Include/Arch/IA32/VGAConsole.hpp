/**
 * @file System/Kernel/Include/Arch/IA32/VGAConsole.hpp
 * @brief IA32 VGA text-mode console driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#include "Logger.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 VGA text-mode console driver.
   */
  class VGAConsole {
    public:
      using Writer = Logger::Writer;

      /**
       * Initializes the console driver.
       */
      static void Initialize();

      /**
       * Writes a character to the console.
       * @param character
       *   The character.
       */
      static void WriteCharacter(char character);

    private:
      /**
       * The number of text-mode columns.
       */
      static constexpr UInt8 _columns = 80;

      /**
       * The number of text-mode rows.
       */
      static constexpr UInt8 _rows = 25;

      /**
       * The default text color (light gray on black).
       */
      static constexpr UInt8 _defaultColor = 0x0F;

      /**
       * The VGA text-mode buffer.
       */
      inline static volatile UInt16* const _buffer
        = reinterpret_cast<volatile UInt16*>(0xB8000);

      /**
       * The current cursor row.
       */
      inline static UInt8 _cursorRow = 0;

      /**
       * The current cursor column.
       */
      inline static UInt8 _cursorColumn = 0;

      /**
       * Saved cursor row for restore.
       */
      inline static UInt8 _cursorSavedRow = 0;

      /**
       * Saved cursor column for restore.
       */
      inline static UInt8 _cursorSavedColumn = 0;

      /**
       * The saved cell value under the cursor.
       */
      inline static UInt16 _cursorSavedCell = 0;

      /**
       * Whether the cursor is currently drawn.
       */
      inline static bool _cursorDrawn = false;

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
      static UInt16 Index(UInt8 row, UInt8 column);

      /**
       * Creates a VGA text-mode entry from a character and color.
       * @param character
       *   The character.
       * @param color
       *   The color attribute.
       * @return
       *   The VGA text-mode entry.
       */
      static UInt16 MakeEntry(char character, UInt8 color);

      /**
       * Hides the cursor.
       */
      static void HideCursor();

      /**
       * Draws the cursor.
       */
      static void DrawCursor();
  };
}
