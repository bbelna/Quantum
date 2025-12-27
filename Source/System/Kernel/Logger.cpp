/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Logger.cpp
 * Kernel logging and tracing interface.
 */

#include <CString.hpp>

#include "Logger.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel {
  using ::Quantum::Format;
  using LogLevel = Kernel::Logger::Level;

  void Logger::Initialize(
    LogLevel minimumLevel,
    Writer** writers,
    Size writerCount
  ) {
    _minimumLevel = minimumLevel;
    _writers = writers;
    _writerCount = writerCount;
  }

  void Logger::Write(LogLevel level, String message) {
    if (level < _minimumLevel) {
      return;
    }

    for (Size i = 0; i < _writerCount; ++i) {
      _writers[i]->Write(message);
    }
  }

  void Logger::WriteFormatted(LogLevel level, String formattedMessage, ...) {
    VariableArgumentsList args;

    if (level >= _minimumLevel) {
      constexpr Size bufferLength = 256;
      char buffer[bufferLength] = {};

      VARIABLE_ARGUMENTS_START(args, formattedMessage);

      Format(
        buffer,
        bufferLength,
        formattedMessage.Data(),
        args
      );

      VARIABLE_ARGUMENTS_END(args);

      for (Size i = 0; i < _writerCount; ++i) {
        _writers[i]->Write(String(buffer));
      }
    }
  }
}
