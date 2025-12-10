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
   * PS/2 keyboard driver class.
   */
  class PS2Keyboard {
    public:
      /**
       * Initializes the PS/2 keyboard IRQ handler (IRQ1).
       */
      static void Initialize();

      /**
       * Returns true if a character is available in the buffer.
       */
      static bool KeyAvailable();

      /**
       * Reads a character from the buffer. Returns 0 if none available.
       */
      static char ReadChar();

      /**
       * Enables or disables echoing of keystrokes to the console.
       */
      static void SetEchoEnabled(bool enabled);
  };
}
