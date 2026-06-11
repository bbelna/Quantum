/**
 * @file Include/Quantum/Threading/Process.hpp
 * @brief Declaration of process-related structures and types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

#define INVALID_PROCESS_ID static_cast<Quantum::Threading::ProcessID>(-1)

namespace Quantum::Threading {
  /**
   * @brief Unique identifier for a process.
   */
  using ProcessID = UInt32;

  /**
   * @brief Permission flags for processes.
   */
  enum class ProcessPermissions : UInt32 {
    /**
     * @brief Process has no special permissions.
     */
    None = 0x0u,

    /**
     * @brief Process has permission to perform port I/O operations.
     */
    PortIO = 0x1u,

    /**
     * @brief Process has permission to handle interrupts.
     */
    Interrupts = 0x2u,
  };

  /**
   * @brief Maximum length of a working directory path, including the null
   *        terminator.
   */
  constexpr Size MaxWorkingDirectoryLength = 256;

  /**
   * @brief Represents a process in the system.
   */
  class Process {
    public:
      /**
       * @brief Gets the @ref ProcessID of the currently running @ref Process.
       * @return Current @ref ProcessID.
       */
      static ProcessID GetCurrentProcessID();

      /**
       * @brief Gets the working directory of the specified process.
       * @param pid The process ID to query.
       * @param outPath Buffer to receive the working directory path.
       * @param outPathSize Size of @p outPath in bytes.
       * @return `true` if the working directory was retrieved successfully.
       */
      static bool GetWorkingDirectory(
        ProcessID pid,
        char* outPath,
        Size outPathSize
      );

      /**
       * @brief Gets the program directory of the specified process (the
       *        directory from which the binary was loaded).
       * @param pid The process ID to query.
       * @param outPath Buffer to receive the program directory path.
       * @param outPathSize Size of @p outPath in bytes.
       * @return `true` if the program directory was retrieved successfully.
       */
      static bool GetProgramDirectory(
        ProcessID pid,
        char* outPath,
        Size outPathSize
      );

      /**
       * @brief Sets the working directory of the specified process.
       * @param pid The process ID to update.
       * @param path The new working directory path (null-terminated).
       */
      static void SetWorkingDirectory(
        ProcessID pid,
        const char* path
      );
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::Threading, ProcessPermissions)
