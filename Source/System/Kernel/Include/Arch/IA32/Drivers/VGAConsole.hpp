/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Drivers/VGAConsole.hpp
 * IA32 VGA text-mode console driver.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/Writer.hpp>
#include <Types/String.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Drivers {
  namespace QK = Quantum::System::Kernel;

  using Writer = QK::Types::Writer;
  using String = QK::Types::String;

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
       * Writes a character to the console.
       * @param character
       *   The character.
       */
      static void WriteCharacter(char character);

      /**
       * Writes a message to the console.
       * @param message
       *   The message.
       */
      static void Write(CString message);

      /**
       * Writes a message followed by a newline to the console.
       * @param message
       *   The message.
       */
      static void WriteLine(CString message = "");

      /**
       * Gets the `Writer` adapter for the `VGAConsole`.
       * @return
       *   The `Writer` adapter.
       */
      static Writer& GetWriter();

      /**
       * `Writer` adapter for `VGAConsole`.
       */
      class WriterAdapter : public Writer {
        public:
          /**
           * Writes a message.
           * @param message The message.
           */
          void Write(String message) override;
      };
  };
}
