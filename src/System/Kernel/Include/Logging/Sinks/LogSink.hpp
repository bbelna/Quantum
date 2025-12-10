//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/LogSink.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Declaration for kernel log sink interface.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>
#include <Logging/Logger.hpp>

namespace Quantum::Kernel::Logging {
  /**
   * Interface for kernel log sinks.
   */
  class LogSink {
    public:
      /**
       * Writes a formattted log message to the sink.
       * @param level The log level.
       * @param message The formatted log message.
       * @param ... Format arguments.
       */
      virtual void Write(
        Logger::Level level,
        String message,
        ...
      ) = 0;

      /**
       * Virtual destructor.
       */
      virtual ~LogSink() = default;
  };
}
