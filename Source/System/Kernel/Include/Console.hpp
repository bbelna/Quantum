/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Console.hpp
 * Architecture-agnostic console driver.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * Architecture-agnostic console driver.
   */
  class Console {
    public:
      /**
       * Initializes the active console backend.
       */
      static void Initialize();

      /**
       * Writes a single character to the console.
       * @param c
       *   The character to write.
       */
      static void WriteCharacter(char c);

      /**
       * Writes a null-terminated string to the console.
       * @param str
       *   The string to write.
       */
      static void Write(CString str);

      /**
       * Writes a buffer with the given length to the console.
       * @param buffer
       *   Pointer to bytes to write.
       * @param length
       *   Number of bytes to write.
       */
      static void Write(CString buffer, UInt32 length);

      /**
       * Writes a line (string followed by newline) to the console.
       * @param str
       *   The string to write.
       */
      static void WriteLine(CString str);

      /**
       * Writes a formatted string to the console.
       * @param format
       *   Formatted string.
       * @param ...
       *   Format arguments.
       */
      static void WriteFormatted(CString format, ...);

      /**
       * Writes a 32-bit value in hexadecimal format to the console.
       * @param value
       *   The value to write.
       */
      static void WriteHex32(UInt32 value);
  };
}
