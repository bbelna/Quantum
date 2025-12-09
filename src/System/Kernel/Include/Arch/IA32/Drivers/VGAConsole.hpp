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
   * IA32 VGA text-mode console driver.
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

      /**
       * Writes a 32-bit value in hexadecimal format to the console.
       */
      static void WriteHex32(uint32 value);
  };
}
