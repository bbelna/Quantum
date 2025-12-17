/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Logger.cpp
 * Kernel logging and tracing interface.
 */

#include <Logger.hpp>
#include <Helpers/CStringHelper.hpp>

namespace Quantum::System::Kernel {
  using Helpers::CStringHelper;

  namespace {
    /**
     * The minimum log level.
     */
    LogLevel _minimumLevel = LogLevel::Trace;

    /**
     * The array of writers.
     */
    Writer** _writers = nullptr;

    /**
     * The number of writers.
     */
    Size _writerCount = 0;
  };

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
      CStringHelper::Format(
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
