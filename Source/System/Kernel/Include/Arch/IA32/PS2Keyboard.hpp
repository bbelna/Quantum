/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/PS2Keyboard.hpp
 * IA32 PS/2 keyboard driver.
 */

#pragma once

namespace Quantum::System::Kernel::Arch::IA32 {
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
       * @return
       *   True if a character is available, false otherwise.
       */
      static bool KeyAvailable();

      /**
       * Reads a character from the buffer.
       * @return
       *   The character read, or 0 if none available.
       */
      static char ReadCharacter();
  };
}
