/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Logging/LogLevel.hpp
 * Log level enumeration.
 */

#pragma once

namespace Quantum::System::Kernel::Types::Logging {
  /**
   * Log levels.
   */
  enum LogLevel {
    Verbose,
    Debug,
    Trace,
    Info,
    Warning,
    Error,
    Panic
  };
}
