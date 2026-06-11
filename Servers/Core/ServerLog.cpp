/**
 * @file Servers/Core/ServerLog.cpp
 * @brief Implements @ref @QSrvCore::ServerLog.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ServerCoreTypes.hpp>

#include "ServerLog.hpp"

namespace Quantum::Servers::Core {
  ServerLog::ServerLog(
    Clients::KernelClient& kernel
  ) :
    _kernel(kernel)
  {
  }

  void ServerLog::Write(
    Quantum::Core::LogLevel level,
    const char* message,
    ...
  ) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);
    _writeV(level, message, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void ServerLog::Trace(const char* message, ...) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);
    _writeV(Quantum::Core::LogLevel::Trace, message, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void ServerLog::Info(const char* message, ...) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);
    _writeV(Quantum::Core::LogLevel::Info, message, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void ServerLog::Warning(const char* message, ...) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);
    _writeV(Quantum::Core::LogLevel::Warning, message, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void ServerLog::Error(const char* message, ...) {
    VariableArgumentsList args;

    VARIABLE_ARGUMENTS_START(args, message);
    _writeV(Quantum::Core::LogLevel::Error, message, args);
    VARIABLE_ARGUMENTS_END(args);
  }

  void ServerLog::_writeV(
    Quantum::Core::LogLevel level,
    const char* message,
    VariableArgumentsList args
  ) {
    if (!message) return;

    char buffer[1024] = {};

    Quantum::Core::CString::FormatV(buffer, 1024, message, args);

    _kernel.WriteLog(level, "%s", buffer);
  }
}
