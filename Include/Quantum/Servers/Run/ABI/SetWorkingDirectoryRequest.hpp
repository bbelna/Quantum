/**
 * @file Include/Quantum/Servers/Run/ABI/SetWorkingDirectoryRequest.hpp
 * @brief Declares @ref SetWorkingDirectoryRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Threading.hpp>

#include "../RunServerConstants.hpp"
#include "../RunServerRequest.hpp"

namespace Quantum::Servers::Run::ABI {
  /**
   * @brief Request to set a process's working directory.
   *        Fire-and-forget; no reply is expected.
   */
  struct SetWorkingDirectoryRequest : public RunServerRequest {
    /**
     * @brief The process ID whose working directory is being set.
     */
    ProcessID PID;

    /**
     * @brief The new working directory path, null-terminated.
     */
    char Path[MaxWorkingDirectoryLength];
  };
}
