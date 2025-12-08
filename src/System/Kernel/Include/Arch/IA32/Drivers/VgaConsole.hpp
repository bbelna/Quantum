//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/VGAConsole.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Declaration for the kernel IA32 VGA console driver.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  /**
   * IA32 VGA text‐mode console driver.
   */
  class VGAConsole {
    public:
      /**
       * Initializes the console driver.
       */
      static void Initialize();

      /**
       * Writes a single character to the console.
       */
      static void WriteChar(char c);

      /**
       * Writes a null-terminated string to the console.
       */
      static void Write(const char* str);

      /**
       * Writes a line (string followed by newline) to the console.
       */
      static void WriteLine(const char* str);

    private:
      /**
       * VGA text‐mode buffer lives at 0xB8000
       * (each entry = 2 bytes: [char][color]).
       */
      static volatile uint16* const buffer;

      /**
       * Foreground/background color byte
       * (high‐4 bits = background, low‐4 bits = FG).
       */
      static constexpr uint8 defaultColor = 0x0F;

      /**
       * The cursor's row.
       */
      static uint8 cursorRow;

      /**
       * The cursor's column.
       */
      static uint8 cursorColumn;

      /**
       * The saved cell under the cursor.
       */
      static uint16 cursorSavedCell;

      /**
       * Whether the cursor is currently drawn.
       */
      static bool cursorDrawn;

      /**
       * Hides the text-mode cursor.
       */
      static void HideCursor();

      /**
       * Draws the text-mode cursor.
       */
      static void DrawCursor();

      /**
       * Converts a row and column into a buffer index.
       * @param r The row (0-24).
       * @param c The column (0-79).
       * @return The index in the VGA text buffer.
       */
      static inline uint16 Index(uint8 r, uint8 c) {
        return (uint16 )(r * 80 + c);
      }

      /**
       * Makes a VGA text-mode buffer entry.
       * @param ch The character.
       * @param color The color byte.
       * @return The VGA buffer entry.
       */
      static inline uint16 MakeEntry(char ch, uint8 color) {
        return (uint16)ch | ((uint16)color << 8);
      }
  };
}
