/**
 * @file System/Kernel/Logger.cpp
 * @brief Kernel logger.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
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

  void Logger::Write(LogLevel level, CString message) {
    if (level < _minimumLevel) {
      return;
    }

    for (Size i = 0; i < _writerCount; ++i) {
      _writers[i]->Write(message);
    }
  }

  void Logger::WriteFormatted(LogLevel level, CString formattedMessage, ...) {
    VariableArgumentsList args;

    if (level >= _minimumLevel) {
      constexpr Size bufferLength = 256;
      char buffer[bufferLength] = {};

      VARIABLE_ARGUMENTS_START(args, formattedMessage);

      Format(
        buffer,
        bufferLength,
        formattedMessage,
        args
      );

      VARIABLE_ARGUMENTS_END(args);

      for (Size i = 0; i < _writerCount; ++i) {
        _writers[i]->Write(buffer);
      }
    }
  }
}
