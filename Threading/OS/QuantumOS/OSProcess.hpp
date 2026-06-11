/**
 * @file Threading/QuantumOSProcess.hpp
 * @brief Implements @ref @QThrd::OS::IOSProcess.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Threading/OS/IOSProcess.hpp>

/**
 * @brief QuantumOS-specific process backend.
 */
namespace Quantum::Threading::OS::QuantumOS {
  /**
   * @brief QuantumOS implementation of @ref @QThrd::OS::IOSProcess.
   */
  class OSProcess : public IOSProcess {
    public:
      /**
       * @brief Gets the process ID of the calling process.
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
