/**
 * @file System/Kernel/Console.cpp
 * @brief Console output handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <CString.hpp>
#include <Types.hpp>

#include "Console.hpp"
#include "CPU.hpp"
#include "Prelude.hpp"

// NOTE: This way of doing arch-independence was removed as part of #25,
// however, we are keeping this here for now since the TextDisplayDevice
// abstraction is what ultimately will replace this code and it is not yet
// implemented. It would be a waste of time to implement an arch wrapper
// for console output when it is going to be removed soon anyway.
#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/VGAConsole.hpp"

namespace Arch = Quantum::System::Kernel::Arch::IA32;
using ConsoleDriver = Arch::VGAConsole;
#endif

namespace Quantum::System::Kernel {
  using ::Quantum::Length;
  using ::Quantum::ToCString;

  Logger::Writer& Console::GetWriter() {
    static WriterAdapter writerAdapter;

    return writerAdapter;
  }

  void Console::WriterAdapter::Write(CString message) {
    if (!message) {
      return;
    }

    WriteLine(message);
  }

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
      WriteUnlocked("0x", 2);
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

          WriteUnlocked(str ? str : "(null)");

          break;
        }

        case 'c': {
          char c = static_cast<char>(VARIABLE_ARGUMENTS(args, int));

          ConsoleDriver::WriteCharacter(c);

          break;
        }

        case 'd': {
          Int32 v = VARIABLE_ARGUMENTS(args, Int32);

          WriteUnlocked(ToCString(v));

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

  void Console::Write(CString string) {
    if (!string) {
      return;
    }

    UInt32 length = static_cast<UInt32>(Length(string));

    Write(string, length);
  }

  void Console::Write(CString string, UInt32 length) {
    if (!string || length == 0) {
      return;
    }

    while (__sync_lock_test_and_set(&_writing, 1) != 0) {
      CPU::Pause();
    }

    WriteUnlocked(string, length);

    __sync_lock_release(&_writing);
  }

  void Console::WriteLine(CString string) {
    if (!string) {
      return;
    }

    UInt32 length = static_cast<UInt32>(Length(string));

    WriteLine(string, length);
  }

  void Console::WriteLine(CString string, UInt32 length) {
    if (!string) {
      return;
    }

    while (__sync_lock_test_and_set(&_writing, 1) != 0) {
      CPU::Pause();
    }

    WriteUnlocked(string, length);
    ConsoleDriver::WriteCharacter('\n');

    __sync_lock_release(&_writing);
  }

  void Console::WriteFormatted(CString format, ...) {
    VariableArgumentsList args;

    while (__sync_lock_test_and_set(&_writing, 1) != 0) {
      CPU::Pause();
    }

    VARIABLE_ARGUMENTS_START(args, format);

    WriteFormattedVariableArguments(format, args);

    VARIABLE_ARGUMENTS_END(args);

    __sync_lock_release(&_writing);
  }

  void Console::WriteUnlocked(CString string, UInt32 length) {
    if (!string || length == 0) {
      return;
    }

    for (UInt32 i = 0; i < length; ++i) {
      ConsoleDriver::WriteCharacter(string[i]);
    }
  }

  void Console::WriteUnlocked(CString string) {
    WriteUnlocked(
      string,
      static_cast<UInt32>(Length(string))
    );
  }
}
