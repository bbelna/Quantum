/**
 * @file Include/Quantum/Servers/Run/ABI/GetWorkingDirectoryRequest.hpp
 * @brief Declares @ref GetWorkingDirectoryRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Threading.hpp>

#include "../RunServerRequest.hpp"

namespace Quantum::Servers::Run::ABI {
  /**
   * @brief Request to get a process's working directory.
   */
  struct GetWorkingDirectoryRequest : public RunServerRequestWithReply {
    /**
     * @brief The process ID whose working directory is being queried.
     */
    ProcessID PID;
  };
}
