/**
 * @file Include/Quantum/Servers/Run/RunServerOperation.hpp
 * @brief Declares @ref RunServerOperation.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::Run {
  /**
   * @brief Operations supported by the run server.
   */
  enum class RunServerOperation : UInt32 {
    /**
     * @brief Load an ELF binary and spawn a new process.
     */
    LoadELF = 1,

    /**
     * @brief Sets the working directory for a process.
     *        Fire-and-forget; no reply is expected.
     */
    SetWorkingDirectory = 2,

    /**
     * @brief Retrieves the working directory for a process.
     */
    GetWorkingDirectory = 3,

    /**
     * @brief Retrieves the program directory for a process (the
     *        directory from which the binary was loaded).
     */
    GetProgramDirectory = 4
  };
}
