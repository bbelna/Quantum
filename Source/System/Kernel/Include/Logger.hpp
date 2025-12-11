//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Logging/Logger.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// The kernel logger.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>
#include <Types/String.hpp>
#include <Types/Writer.hpp>

namespace Quantum::Kernel {
  using String = Types::String;
  using Writer = Types::Writer;

  /**
   * The kernel logger class.
   */
  class Logger {
    public:
      /**
       * Log levels.
       */
      enum class Level : UInt8 {
        Trace,
        Info,
        Warning,
        Error,
        Panic
      };

      /**
       * Initializes the logger with the given sinks and minimum log level.
       * @param minimumLevel The minimum log level.
       * @param writers Array of writers.
       * @param writerCount Number of writers.
       */
      static void Initialize(
        Level minimumLevel,
        Writer** writers,
        Size writerCount
      );

      /**
       * Writes a message to the kernel log.
       * @param level The log level.
       * @param message The message to write.
       */
      static void Write(Level level, String message);

      /**
       * Writes a formatted message to the kernel log.
       * @param level The log level.
       * @param formattedMessage The formatted message to write.
       * @param ... Format arguments.
       */
      static void WriteFormatted(Level level, String formattedMessage, ...);
  };
}
