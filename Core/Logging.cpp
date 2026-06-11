/**
 * @file Core/Logging.cpp
 * @brief Implements @ref @QCore::Log and @ref @QCore::LogSink.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "CString.hpp"
#include "Definitions.hpp"
#include "Logging.hpp"

namespace Quantum::Core {
  void LogSink::Write(LogLevel level, const char* message) {
    if (!_put || level < _minLevel || !message) return;

    for (Size i = 0; message[i] != '\0'; ++i) _put(message[i]);

    _put('\n');
  }

  void LogSink::Write(
    LogLevel level,
    Size length,
    const char* message
  ) {
    if (!_put || level < _minLevel || !message || length == 0) return;

    for (Size i = 0; i < length; ++i) _put(message[i]);

    _put('\n');
  }

  void Log::Write(LogLevel level, const char* message, ...) {
    if (!_sinks || _sinkCount == 0 || !message) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(level, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }

  void Log::Write(
    LogLevel level,
    const char* message,
    VariableArgumentsList args
  ) {
    if (message == nullptr) return;

    char buffer[CString::FormatBufferSize] = {};

    CString::FormatV(
      buffer,
      sizeof(buffer),
      message,
      args
    );

    for (Size i = 0; i < _sinkCount; ++i) {
      _sinks[i]->Write(level, buffer);
    }
  }

  void Log::Write(LogLevel level, Size length, const char* message) {
    if (_sinks == nullptr || _sinkCount == 0 || message == nullptr)
      return;

    char buffer[CString::FormatBufferSize] = {};

    CString::Copy(message, buffer, sizeof(buffer));

    for (Size i = 0; i < _sinkCount; ++i) {
      _sinks[i]->Write(level, buffer);
    }
  }

  void Log::Debug(const char* message, ...) {
    if (message == nullptr) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(LogLevel::Debug, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }

  void Log::Trace(const char* message, ...) {
    if (message == nullptr) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(LogLevel::Trace, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }

  void Log::Info(const char* message, ...) {
    if (message == nullptr) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(LogLevel::Info, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }

  void Log::Warning(const char* message, ...) {
    if (message == nullptr) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(LogLevel::Warning, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }

  void Log::Error(const char* message, ...) {
    if (message == nullptr) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(LogLevel::Error, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }

  void Log::Critical(const char* message, ...) {
    if (message == nullptr) return;

    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);

    Write(LogLevel::Critical, message, args);

    VARIABLE_ARGUMENTS_END(args);
  }
}
