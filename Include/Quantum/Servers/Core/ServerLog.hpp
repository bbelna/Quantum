/**
 * @file Include/Quantum/Servers/Core/ServerLog.hpp
 * @brief Declares @ref @QSrvCore::ServerLog.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Clients/KernelClient.hpp>

namespace Quantum::Servers::Core {
  /**
   * @brief Server-side logging facade that routes messages through
   *        @ref KernelClient.
   *
   * Provides a `printf`-style logging API for server code. All messages
   * are forwarded to @ref KernelClient::WriteLog, keeping servers
   * decoupled from the raw kernel ABI.
   *
   * @code
   *   ServerLog log(kernel);
   *   log.Info("Mounted volume %s", volumeID);
   *   log.Warning("Request too small (%u bytes)", size);
   * @endcode
   */
  class ServerLog {
    public:
      /**
       * @brief Creates a new @ref ServerLog.
       * @param kernel Reference to the @ref KernelClient for log
       *                     output. Must outlive this instance.
       */
      explicit ServerLog(Clients::KernelClient& kernel);

      /**
       * @brief Writes a formatted message at the specified log level.
       * @param level The log level.
       * @param message Null-terminated format string.
       * @param ... Variable arguments for formatting.
       */
      void Write(Quantum::Core::LogLevel level, const char* message, ...);

      /**
       * @brief Writes a formatted trace-level message.
       * @param message Null-terminated format string.
       * @param ... Variable arguments for formatting.
       */
      void Trace(const char* message, ...);

      /**
       * @brief Writes a formatted info-level message.
       * @param message Null-terminated format string.
       * @param ... Variable arguments for formatting.
       */
      void Info(const char* message, ...);

      /**
       * @brief Writes a formatted warning-level message.
       * @param message Null-terminated format string.
       * @param ... Variable arguments for formatting.
       */
      void Warning(const char* message, ...);

      /**
       * @brief Writes a formatted error-level message.
       * @param message Null-terminated format string.
       * @param ... Variable arguments for formatting.
       */
      void Error(const char* message, ...);

    private:
      /**
       * @brief Reference to the @ref KernelClient for log output.
       */
      Clients::KernelClient& _kernel;

      /**
       * @brief Formats and sends a log message to the kernel.
       * @param level The log level.
       * @param message Null-terminated format string.
       * @param args Pre-initialized variable arguments list.
       */
      void _writeV(
        Quantum::Core::LogLevel level,
        const char* message,
        VariableArgumentsList args
      );
  };
}
