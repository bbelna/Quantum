/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Console.cpp
 * Architecture-agnostic console driver.
 */

#include <Console.hpp>
#include <Helpers/CStringHelper.hpp>
#include <Types.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/VGAConsole.hpp>

namespace Arch = Quantum::System::Kernel::Arch::IA32;
using ConsoleDriver = Arch::VGAConsole;
#endif

namespace Quantum::System::Kernel {
  using Helpers::CStringHelper;

  void Console::WriteUnsigned(UInt32 value, UInt32 base, bool prefixHex) {
    if (base < 2 || base > 16) {
      return;
    }

    char buffer[16] = {};
    Size idx = 0;
    CString digits = "0123456789ABCDEF";

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

  void Console::WriteFormattedVariableArguments(
    CString format,
    VariableArgumentsList args
  ) {
    if (!format) {
      return;
    }

    for (CString p = format; *p != '\0'; ++p) {
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
          CString str = VARIABLE_ARGUMENTS(args, CString);

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

  void Console::Initialize() {
    ConsoleDriver::Initialize();
  }

  void Console::WriteCharacter(char c) {
    ConsoleDriver::WriteCharacter(c);
  }

  void Console::Write(CString str) {
    if (!str) {
      return;
    }

    UInt32 length = static_cast<UInt32>(CStringHelper::Length(str));

    Write(str, length);
  }

  void Console::Write(CString buffer, UInt32 length) {
    if (!buffer || length == 0) {
      return;
    }

    for (UInt32 i = 0; i < length; ++i) {
      ConsoleDriver::WriteCharacter(buffer[i]);
    }
  }

  void Console::WriteLine(CString str) {
    Write(str);
    WriteCharacter('\n');
  }

  void Console::WriteFormatted(CString format, ...) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, format);
    WriteFormattedVariableArguments(format, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void Console::WriteHex32(UInt32 value) {
    WriteUnsigned(value, 16, true);
  }
}
