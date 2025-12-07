//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Include/Drivers/Console.hpp
// Declaration of the kernel's Console class.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Drivers {
  class Console {
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
      static void WriteString(const char* str);

    private:
      /**
       * VGA text‐mode buffer lives at 0xB8000
       * (each entry = 2 bytes: [char][color]).
       */
      static volatile uint16* const buffer;

      /**
       * Current cursor row in the text buffer.
       */
      static uint8 row;

      /**
       * Current cursor column in the text buffer.
       */
      static uint8 col;

      /**
       * Foreground/background color byte
       * (high‐4 bits = background, low‐4 bits = FG).
       */
      static constexpr uint8 defaultColor = 0x0F;

      static uint8 cursorRow;
      static uint8 cursorColumn;
      static uint16 cursorSavedCell;
      static bool cursorDrawn;

      static void HideCursor();
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

      static inline uint16 MakeEntry(char ch, uint8 color) {
        return (uint16)ch | ((uint16)color << 8);
      }
  };
}
