/**
 * @file Kernel/Logging/SpinlockLog.hpp
 * @brief Declares @ref @QKrnl::Logging::SpinlockLog.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/Spinlock.hpp>
#include <Drivers/Graphics/GraphicsDriverTypes.hpp>

namespace Quantum::Kernel::Logging {
  /**
   * @brief @ref Log implementation that uses a @ref Spinlock to protect
   *        @ref Log writes.
   */
  class SpinlockLog : public Log {
    public:
      /**
       * @brief Creates a new @ref SpinlockLog.
       * @param sinks Pointer to an array of @ref LogSink pointers.
       * @param sinkCount Number of sinks in @p sinks.
       */
      SpinlockLog(
        LogSink** sinks,
        Size sinkCount
      ) : Log(sinks, sinkCount) {}

      /**
       * @brief Logs a message at the specified log level to all configured
       *        sinks.
       * @param level The log level.
       * @param message The message to log.
       * @param args Variable arguments list for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Write(
        LogLevel level,
        const char* message,
        VariableArgumentsList args
      ) override {
        _spinlock.Acquire();

        CString::FormatV(
          _buffer,
          sizeof(_buffer),
          message,
          args
        );

        for (
          Size sinkIndex = 0;
          sinkIndex < _sinkCount;
          ++sinkIndex
        ) {
          _sinks[sinkIndex]->Write(
            level,
            _buffer
          );
        }

        _spinlock.Release();
      }

      /**
       * @brief Logs a pre-formatted message at the specified log level to all
       *        configured sinks.
       * @param level The log level.
       * @param length The length of the message.
       * @param message The message to log.
       */
      void Write(
        LogLevel level,
        Size length,
        const char* message
      ) override {
        _spinlock.Acquire();

        for (
          Size sinkIndex = 0;
          sinkIndex < _sinkCount;
          ++sinkIndex
        ) {
          _sinks[sinkIndex]->Write(
            level,
            length,
            message
          );
        }

        _spinlock.Release();
      }

    private:
      /**
       * @brief @ref Spinlock to protect concurrent access to the buffer and
       *        sinks.
       */
      Spinlock<UInt32> _spinlock;

      /**
       * @brief Buffer for formatting messages.
       *
       * Shared across all log calls, but protected by @ref _spinlock to ensure
       * @ref Thread safety.
       */
      char _buffer[CString::FormatBufferSize];
  };
}
