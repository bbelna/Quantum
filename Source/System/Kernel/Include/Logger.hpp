/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Logging/Logger.hpp
 * Kernel logger for writing log messages.
 */

#pragma once

#include "String.hpp"
#include "Types.hpp"

namespace Quantum::System::Kernel {
  /**
   * The kernel logger class.
   */
  class Logger {
    public:
      /**
       * Log levels.
       */
      enum class Level : UInt32 {
        Verbose = 0,
        Debug = 100,
        Trace = 200,
        Info = 300,
        Warning = 400,
        Error = 500,
        Panic = 600
      };

      /**
       * Abstract writer interface for writing data.
       */
      class Writer {
        public:
          /**
           * Writes a message.
           * @param message
           *   The message.
           */
          virtual void Write(String message);

          /**
           * Virtual destructor.
           */
          virtual ~Writer() = default;
      };

      /**
       * Initializes the logger with the given writers and minimum log level.
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

    private:
      /**
       * The minimum log level.
       */
      static inline Level _minimumLevel = Level::Debug;

      /**
       * The array of writers.
       */
      static inline Writer** _writers = nullptr;

      /**
       * The number of writers.
       */
      static inline Size _writerCount = 0;
  };
}
