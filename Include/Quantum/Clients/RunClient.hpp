/**
 * @file Include/Quantum/Clients/RunClient.hpp
 * @brief Declares @ref @QClients::RunClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Kernel/Memory/MemoryTypes.hpp>
#include <Quantum/Types.hpp>

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the run server.
   *
   * Wraps the ELF loading IPC call behind a clean API.
   */
  class RunClient {
    public:
      /**
       * @brief Creates a new @ref RunClient instance.
       */
      RunClient() = default;

      /**
       * @brief Loads an ELF binary via the run server and spawns a process.
       * @param name Human-readable name for the new process.
       * @param workingDirectory Initial working directory for the new process.
       * @param elfData Pointer to the raw ELF binary data.
       * @param elfSize Size of the ELF binary in bytes.
       * @param argumentCount Number of command-line arguments.
       * @param arguments Array of null-terminated argument strings.
       * @param streamCount Number of inherited stream buffer IDs (0 to 3).
       * @param streamBufferIDs Array of shared buffer IDs for inherited
       *                        streams, or `nullptr` if none.
       * @return The spawned process ID, or -1 on failure.
       */
      ProcessID LoadELF(
        const char* name,
        const char* workingDirectory,
        const void* elfData,
        Size elfSize,
        Size argumentCount = 0,
        const char* const* arguments = nullptr,
        UInt8 streamCount = 0,
        const Quantum::Kernel::Memory::SharedBufferID* streamBufferIDs
          = nullptr
      );

      /**
       * @brief Sets the working directory for a process.
       * @param pid The process ID.
       * @param path The new working directory path (null-terminated).
       */
      void SetWorkingDirectory(ProcessID pid, const char* path);

      /**
       * @brief Retrieves the working directory for a process.
       * @param pid The process ID.
       * @param outPath Buffer to receive the working directory path.
       * @param outPathSize Size of @p outPath in bytes.
       * @return `true` if the process was found and the path was copied.
       */
      bool GetWorkingDirectory(
        ProcessID pid,
        char* outPath,
        Size outPathSize
      );

      /**
       * @brief Retrieves the program directory for a process (the
       *        directory from which the binary was loaded).
       * @param pid The process ID.
       * @param outPath Buffer to receive the program directory path.
       * @param outPathSize Size of @p outPath in bytes.
       * @return `true` if the process was found and the path was copied.
       */
      bool GetProgramDirectory(
        ProcessID pid,
        char* outPath,
        Size outPathSize
      );
  };
}
