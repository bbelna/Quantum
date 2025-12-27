/**
 * @file System/Kernel/Include/Logger.hpp
 * @brief Kernel logger.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

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
           *   The message to write.
           */
          virtual void Write(CString message);

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
      static void Write(Level level, CString message);

      /**
       * Writes a formatted message to the kernel log.
       * @param level
       *   The log level.
       * @param formattedMessage
       *   The formatted message to write.
       * @param ...
       *   Format arguments.
       */
      static void WriteFormatted(Level level, CString formattedMessage, ...);

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
