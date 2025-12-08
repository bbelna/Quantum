//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Drivers/Console.hpp
// Brandon Belna - MIT License
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

      /**
       * Writes a 32-bit value in hexadecimal format to the console.
       */
      static void WriteHex32(uint32 value);
  };
}
