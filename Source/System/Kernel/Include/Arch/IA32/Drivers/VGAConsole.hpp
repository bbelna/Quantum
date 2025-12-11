//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Drivers/VGAConsole.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Declaration for the kernel IA32 VGA console driver.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>
#include <Types/Writer.hpp>
#include <Types/String.hpp>

namespace Quantum::Kernel::Arch::IA32::Drivers {
  using Writer = Types::Writer;
  using String = Types::String;

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
       * @param character The character.
       */
      static void WriteCharacter(char character);

      /**
       * Writes a message to the console.
       * @param message The message.
       */
      static void Write(CString message);

      /**
       * Writes a message followed by a newline to the console.
       * @param message The message.
       */
      static void WriteLine(CString message = "");

      /**
       * Gets the `Writer` adapter for the `VGAConsole`.
       * @return The `Writer` adapter.
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
