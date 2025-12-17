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
    Verbose = 0,
    Debug = 100,
    Trace = 200,
    Info = 300,
    Warning = 400,
    Error = 500,
    Panic = 600
  };
}
