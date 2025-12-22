/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/VGAConsole.hpp
 * IA32 VGA text-mode console driver.
 */

#pragma once

#include <Logger.hpp>
#include <Prelude.hpp>
#include <Types.hpp>
#include <String.hpp>

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
      static volatile UInt16* const _buffer;

      /**
       * The current cursor row.
       */
      static UInt8 _cursorRow;

      /**
       * The current cursor column.
       */
      static UInt8 _cursorColumn;

      /**
       * Saved cursor row for restore.
       */
      static UInt8 _cursorSavedRow;

      /**
       * Saved cursor column for restore.
       */
      static UInt8 _cursorSavedColumn;

      /**
       * The saved cell value under the cursor.
       */
      static UInt16 _cursorSavedCell;

      /**
       * Whether the cursor is currently drawn.
       */
      static bool _cursorDrawn;

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
