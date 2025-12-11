//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/Keyboard.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// PS/2 keyboard driver for IA32.
//------------------------------------------------------------------------------

#pragma once

namespace Quantum::Kernel::Arch::IA32::Drivers {
  /**
   * IA32 PS/2 (IRQ1) keyboard driver.
   */
  class PS2Keyboard {
    public:
      /**
       * Initializes the PS/2 keyboard driver.
       */
      static void Initialize();

      /**
       * Checks if a character is available in the buffer.
       * @return True if a character is available, false otherwise.
       */
      static bool KeyAvailable();

      /**
       * Reads a character from the buffer.
       * @return The character read, or 0 if none available.
       */
      static char ReadCharacter();
  };
}
