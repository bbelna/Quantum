//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Logger.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Kernel logging and tracing interface.
//------------------------------------------------------------------------------

#include <Helpers/CStringHelper.hpp>
#include <Logger.hpp>
#include <Types.hpp>
#include <Types/String.hpp>
#include <Types/Writer.hpp>

namespace Quantum::Kernel {
  using CStringHelper = Helpers::CStringHelper;
  using Level = Logger::Level;
  using String = Types::String;
  using Writer = Types::Writer;

  namespace {
    /**
     * The minimum log level.
     */
    Level _minimumLevel = Level::Trace;

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
    Level minimumLevel,
    Writer** writers,
    Size writerCount
  ) {
    _minimumLevel = minimumLevel;
    _writers = writers;
    _writerCount = writerCount;
  }

  void Logger::Write(Level level, String message) {
    if (level <= _minimumLevel) {
      return;
    }

    for (Size i = 0; i < _writerCount; ++i) {
      _writers[i]->Write(message);
    }
  }

  void Logger::WriteFormatted(Level level, String formattedMessage, ...) {
    VariableArgumentsList args;

    if (level <= _minimumLevel) {
      return;
    }

    constexpr Size bufferLength = 256;
    char buffer[bufferLength] = {};

    VARIABLE_ARGUMENTS_START(args, formattedMessage);

    CStringHelper::Format(buffer, bufferLength, formattedMessage.Data(), args);

    VARIABLE_ARGUMENTS_END(args);

    for (Size i = 0; i < _writerCount; ++i) {
      _writers[i]->Write(String(buffer));
    }
  }
}
