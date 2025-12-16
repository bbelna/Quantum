/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Logging/Logger.hpp
 * Kernel logger for writing log messages.
 */

#pragma once

#include <Types/Logging/Level.hpp>
#include <Types/Primitives.hpp>
#include <Types/String.hpp>
#include <Types/Writer.hpp>

namespace Quantum::System::Kernel {
  using Level = Types::Logging::Level;
  using String = Types::String;
  using Writer = Types::Writer;

  /**
   * The kernel logger class.
   */
  class Logger {
    public:
      /**
       * Initializes the logger with the given sinks and minimum log level.
       * @param minimumLevel
       *   The minimum log level.
       * @param writers
       *   Array of writers.
       * @param writerCount
       *   Number of writers.
       */
      static void Initialize(
        Level minimumLevel,
        Writer** writers,
        Size writerCount
      );

      /**
       * Writes a message to the kernel log.
       * @param level
       *   The log level.
       * @param message
       *   The message to write.
       */
      static void Write(Level level, String message);

      /**
       * Writes a formatted message to the kernel log.
       * @param level
       *   The log level.
       * @param formattedMessage
       *   The formatted message to write.
       * @param ...
       *   Format arguments.
       */
      static void WriteFormatted(Level level, String formattedMessage, ...);
  };
}
