//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Logger.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Kernel logging and tracing interface.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Logging {
  class LogSink;

  /**
   * Kernel logger class for logging and tracing messages.
   */
  class Logger {
    public:
      /**
       * Log levels.
       */
      enum class Level {
        Trace,
      };

      /**
       * Initializes the logger with the given sinks and minimum log level.
       * @param minimumLevel The minimum log level.
       * @param sinks Array of log sinks.
       * @param sinkCount Number of log sinks.
       */
      static void Initialize(
        Level minimumLevel,
        LogSink** sinks,
        Size sinkCount
      );

      /**
       * Logs a formatted message to the kernel log.
       * @param level The log level.
       * @param format Formatted string.
       * @param ... Format arguments.
       */
      static void Log(Level level, String format, ...);
  };
}
