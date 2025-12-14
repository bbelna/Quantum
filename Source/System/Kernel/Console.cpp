//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Drivers/Console.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// The kernel's console driver.
//------------------------------------------------------------------------------

#include <Console.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Types.hpp>

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Drivers/VGAConsole.hpp>

  namespace Arch = Quantum::Kernel::Arch::IA32;
  using ConsoleDriver = Arch::Drivers::VGAConsole;
#else
  #error "No architecture selected for console driver"
#endif

namespace Quantum::Kernel {
  using Helpers::CStringHelper;

  namespace {
    /**
     * Converts an unsigned integer to a string in the given base.
     * @param value Number to convert.
     * @param base Conversion base (10 or 16).
     * @param prefixHex Whether to prefix 0x for hex.
     */
    void WriteUnsigned(UInt32 value, UInt32 base, bool prefixHex = false) {
      if (base < 2 || base > 16) {
        return;
      }

      char buffer[16] = {};
      Size idx = 0;
      const char* digits = "0123456789ABCDEF";

      do {
        buffer[idx++] = digits[value % base];
        value /= base;
      } while (value > 0 && idx < sizeof(buffer));

      if (prefixHex) {
        ConsoleDriver::Write("0x");
      }

      while (idx > 0) {
        ConsoleDriver::WriteCharacter(buffer[--idx]);
      }
    }

    /**
     * Writes a formatted string to the console using a variable argument list.
     * @param format Format string.
     * @param args Variable argument list.
     */
    void WriteFormattedVariableArguments(
      const char* format,
      VariableArgumentsList args
    ) {
      if (!format) {
        return;
      }

      for (const char* p = format; *p != '\0'; ++p) {
        if (*p != '%') {
          ConsoleDriver::WriteCharacter(*p);

          continue;
        }

        ++p;

        if (*p == '\0') {
          break;
        }

        switch (*p) {
          case 's': {
            const char* str = VARIABLE_ARGUMENTS(args, const char*);

            ConsoleDriver::Write(str ? str : "(null)");

            break;
          }
          case 'c': {
            char c = static_cast<char>(VARIABLE_ARGUMENTS(args, int));

            ConsoleDriver::WriteCharacter(c);

            break;
          }
          case 'd': {
            Int32 v = VARIABLE_ARGUMENTS(args, Int32);

            ConsoleDriver::Write(CStringHelper::ToCString(v));

            break;
          }
          case 'u': {
            UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

            WriteUnsigned(v, 10);

            break;
          }
          case 'x': {
            UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

            WriteUnsigned(v, 16, false);

            break;
          }
          case 'p': {
            UInt32 v = VARIABLE_ARGUMENTS(args, UInt32);

            WriteUnsigned(v, 16, true);

            break;
          }
          case '%': {
            ConsoleDriver::WriteCharacter('%');

            break;
          }
          default: {
            ConsoleDriver::WriteCharacter('%');
            ConsoleDriver::WriteCharacter(*p);

            break;
          }
        }
      }
    }
  }

  void Console::Initialize() {
    ConsoleDriver::Initialize();
  }

  void Console::WriteCharacter(char c) {
    ConsoleDriver::WriteCharacter(c);
  }

  void Console::Write(const char* str) {
    ConsoleDriver::Write(str);
  }

  void Console::WriteLine(const char* str) {
    Write(str);
    WriteCharacter('\n');
  }

  void Console::WriteFormatted(const char* format, ...) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, format);
    WriteFormattedVariableArguments(format, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void Console::WriteHex32(UInt32 value) {
    WriteUnsigned(value, 16, true);
  }
}
