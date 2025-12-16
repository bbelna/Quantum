/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Logging/Level.hpp
 * Log level enumeration.
 */

#pragma once

namespace Quantum::System::Kernel::Types::Logging {
  /**
   * Log levels.
   */
  enum Level {
    Verbose,
    Debug,
    Trace,
    Info,
    Warning,
    Error,
    Panic
  };
}
