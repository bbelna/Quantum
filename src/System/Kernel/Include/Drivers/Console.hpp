//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Drivers/Console.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Architecture-agnostic console interface for the kernel.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Drivers {
  class Console {
    public:
      /**
       * Initializes the active console backend.
       */
      static void Initialize();

      /**
       * Writes a single character to the console.
       * @param c The character to write.
       */
      static void WriteChar(char c);

      /**
       * Writes a null-terminated string to the console.
       * @param str The string to write.
       */
      static void Write(const char* str);

      /**
       * Writes a line (string followed by newline) to the console.
       * @param str The string to write.
       */
      static void WriteLine(const char* str);

      /**
       * Writes a formatted string to the console.
       * Supported specifiers: %s, %c, %d, %u, %x, %p, %%.
       * @param format Formatted string.
       * @param ... Format arguments.
       */
      static void WriteFormatted(const char* format, ...);

      /**
       * Writes a 32-bit value in hexadecimal format to the console.
       * @param value The value to write.
       */
      static void WriteHex32(UInt32 value);
  };
}
