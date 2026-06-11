/**
 * @file Servers/Run/Core/Process/ProcessEntry.hpp
 * @brief Declares @ref @QRunSrv::Process::ProcessEntry.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Types.hpp>

#include <RunServerConstants.hpp>

namespace Quantum::Servers::Run::Core::Process {
  /**
   * @brief Per-process metadata tracked by the run server.
   */
  struct ProcessEntry {
    /**
     * @brief The @ref ProcessID. A value of `0` indicates an unused slot.
     */
    ProcessID PID = 0;

    /**
     * @brief The @ref Process working directory path (volume + directory).
     */
    char WorkingDirectory[MaxWorkingDirectoryLength] = {};

    /**
     * @brief The directory from which the process binary was loaded
     *        (volume + directory, no trailing slash).
     */
    char ProgramDirectory[MaxWorkingDirectoryLength] = {};
  };
}
