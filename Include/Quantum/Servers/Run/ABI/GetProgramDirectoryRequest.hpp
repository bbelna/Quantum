/**
 * @file Include/Quantum/Servers/Run/ABI/GetProgramDirectoryRequest.hpp
 * @brief Declares @ref GetProgramDirectoryRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Types.hpp>

#include "../RunServerRequest.hpp"

namespace Quantum::Servers::Run::ABI {
  /**
   * @brief Request to get a process's program directory (the directory
   *        from which the binary was loaded).
   */
  struct GetProgramDirectoryRequest : public RunServerRequestWithReply {
    /**
     * @brief The process ID whose program directory is being queried.
     */
    ProcessID PID;
  };
}
