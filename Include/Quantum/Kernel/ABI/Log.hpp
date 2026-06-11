/**
 * @file Include/Quantum/Kernel/ABI/Log.hpp
 * @brief Declaration of the kernel log ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/CString.hpp>
#include <Quantum/Core/Definitions.hpp>
#include <Quantum/Kernel/Arch.hpp>
#include <Quantum/Core/Logging.hpp>
#include <Quantum/Core/Types.hpp>

#include "ABI.hpp"

/**
 * @brief ABI functions for kernel logging.
 */
namespace Quantum::Kernel::ABI::Log {
  /**
   * @brief Writes a message to the kernel log.
   * @param level The log level.
   * @param message Null-terminated string to write.
   * @param ... Variable arguments for formatting.
   */
  inline void Write(
    Quantum::Core::LogLevel level,
    const char* message,
    ...
  ) {
    if (!message) return;

    VariableArgumentsList args;

    char buffer[1024] = {};

    VARIABLE_ARGUMENTS_START(args, message);

    Core::CString::FormatV(buffer, 1024, message, args);

    VARIABLE_ARGUMENTS_END(args);

    UInt32 length = Core::CString::Length(buffer);

    if (length == 0) return;

    Invoke(
      KernelOperation::Log_Write,
      static_cast<UInt32>(level),
      reinterpret_cast<UInt32>(buffer),
      length
    );
  }

  /**
   * @brief Sets the kernel log level.
   * @param level The new log level to set.
   */
  inline void SetLevel(Quantum::Core::LogLevel level) {
    Invoke(KernelOperation::Log_SetLevel, static_cast<UInt32>(level));
  }
}
