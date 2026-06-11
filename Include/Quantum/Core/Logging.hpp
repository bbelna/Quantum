/**
 * @file Include/Quantum/Core/Logging.hpp
 * @brief Declaration of @ref @QCore::Log, @ref @QCore::LogSink,
 *        and related types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Enumerates log levels.
   */
  enum class LogLevel : UInt32 {
    /**
     * @brief Debug log level.
     *
     * This is the most verbose log level and is intended primarily for
     * development and debugging purposes.
     */
    Debug = 0,

    /**
     * @brief Trace log level.
     *
     * This level is used for tracing the flow of execution and is less verbose
     * than @ref Debug.
     */
    Trace = 10,

    /**
     * @brief Info log level.
     *
     * This level is the default and used for general informational messages.
     */
    Info = 20,

    /**
     * @brief Warning log level.
     *
     * This level is used for potentially harmful situations.
     */
    Warning = 30,

    /**
     * @brief Error log level.
     *
     * This level is used for error conditions.
     */
    Error = 40,

    /**
     * @brief Critical log level.
     *
     * This level is used for critical conditions that lead to instability or
     * crashes.
     */
    Critical = 50
  };

  /**
   * @brief Logs messages to a specified output sink.
   */
  class LogSink {
    public:
      /**
       * @brief Creates a new @ref LogSink instance with the specified
       *        character output function and minimum log level.
       * @param put Function to output a single character.
       * @param minLevel The minimum log level.
       */
      explicit LogSink(
        void (*put)(char character),
        LogLevel minLevel
      ) : _minLevel(minLevel), _put(put) {}

      /**
       * @brief Destroys this @ref LogSink instance.
       */
      virtual ~LogSink() = default;

      /**
       * @brief Logs a message at the specified log level.
       * @param level The log level.
       * @param message The message to log.
       */
      virtual void Write(LogLevel level, const char* message);

      /**
       * @brief Logs a message at the specified log level.
       * @param level The log level.
       * @param length The length of the message.
       * @param message The message to log.
       */
      virtual void Write(
        LogLevel level,
        Size length,
        const char* message
      );

      /**
       * @brief Sets the minimum log level for this log sink, given the minimum
       *        level has not been locked.
       * @param level The minimum log level to set.
       *
       * The minumum level can be locked and unlocked with @ref LockMinLevel
       * and @ref UnlockMinLevel. When the minimum level is locked, calls to
       * @ref SetMinLevel will have no effect until it is unlocked.
       *
       * This allows critical log sinks to ensure that their minimum level
       * cannot be raised above a certain threshold by other parts of the
       * system, while still allowing non-critical log sinks to adjust their
       * minimum level as needed.
       */
      virtual void SetMinLevel(LogLevel level) {
        if (!_minLevelLocked) _minLevel = level;
      }

      /**
       * @brief Locks the minimum log level, preventing it from being changed
       *        until it is unlocked.
       *
       * See @ref SetMinLevel for details regarding the minimum log level
       * lock.
       */
      void LockMinLevel() {
        _minLevelLocked = true;
      }

      /**
       * @brief Unlocks the minimum log level, allowing it to be changed again.
       *
       * See @ref SetMinLevel for details regarding the minimum log level
       * lock.
       */
      void UnlockMinLevel() {
        _minLevelLocked = false;
      }

    protected:
      /**
       * @brief The current minimum log level.
       */
      LogLevel _minLevel;

      /**
       * @brief Function to output a single character.
       */
      void (*_put)(char character);

      /**
       * @brief Indicates whether the minimum log level has been locked.
       * @see @ref SetMinLevel, @ref LockMinLevel,
       *      @ref UnlockMinLevel
       */
      bool _minLevelLocked = false;
  };

  /**
   * @brief Logs formatted messages to @ref LogSink instances.
   */
  class Log {
    public:
      /**
       * @brief Creates a new @ref Log instance with the specified array of
       *        log sinks.
       * @param sinks Array of log sink pointers.
       * @param sinkCount Number of sinks in the array.
       */
      explicit Log(
        LogSink** sinks,
        Size sinkCount
      ) : _sinks(sinks), _sinkCount(sinkCount) {}

      /**
       * @brief Destroys this @ref Log instance.
       */
      virtual ~Log() = default;

      /**
       * @brief Sets the array of log sinks.
       * @param sinks Array of log sink pointers.
       * @param sinkCount Number of sinks in the array.
       */
      virtual void SetSinks(LogSink** sinks, Size sinkCount) {
        _sinks = sinks;
        _sinkCount = sinkCount;
      }

      /**
       * @brief Sets the minimum log level for all configured sinks.
       * @param level The minimum log level to set.
       */
      void SetMinLevel(LogLevel level) {
        for (Size i = 0; i < _sinkCount; i++) {
          if (_sinks[i]) {
            _sinks[i]->SetMinLevel(level);
          }
        }
      }

      /**
       * @brief Logs a formatted message at the specified log level to all
       *        configured sinks.
       * @param level The log level.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Write(LogLevel level, const char* message, ...);

      /**
       * @brief Logs a formatted message at the specified log level to all
       *        configured sinks.
       * @param level The log level.
       * @param message The message to log.
       * @param args Variable arguments list for formatting.
       *
       * Supports `printf`-style formatting.
       */
      virtual void Write(
        LogLevel level,
        const char* message,
        VariableArgumentsList args
      );

      /**
       * @brief Logs a message at the specified log level to all configured
       *        sinks.
       * @param level The log level.
       * @param length The length of the message.
       * @param message The message to log.
       */
      virtual void Write(LogLevel level, Size length, const char* message);

      /**
       * @brief Logs a debug-level message.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Debug(const char* message, ...);

      /**
       * @brief Logs a trace-level message.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Trace(const char* message, ...);

      /**
       * @brief Logs an info-level message.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Info(const char* message, ...);

      /**
       * @brief Logs a warning-level message.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Warning(const char* message, ...);

      /**
       * @brief Logs an error-level message.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Error(const char* message, ...);

      /**
       * @brief Logs a critical-level message.
       * @param message The message to log.
       * @param ... Additional arguments for formatting.
       *
       * Supports `printf`-style formatting.
       */
      void Critical(const char* message, ...);

    protected:
      /**
       * @brief The size of the format buffer.
       */
      static constexpr Size _formatBufferSize = 1024;

      /**
       * @brief Array of log sinks.
       */
      LogSink** _sinks;

      /**
       * @brief Number of sinks in the array.
       */
      Size _sinkCount;
  };
}
