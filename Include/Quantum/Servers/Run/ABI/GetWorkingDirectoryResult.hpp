/**
 * @file Include/Quantum/Servers/Run/ABI/GetWorkingDirectoryResult.hpp
 * @brief Declares @ref GetWorkingDirectoryResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "../RunServerConstants.hpp"

namespace Quantum::Servers::Run::ABI {
  /**
   * @brief Result of a @ref RunServerOperation::GetWorkingDirectory request.
   */
  struct GetWorkingDirectoryResult {
    /**
     * @brief `true` if the process was found; `false` otherwise.
     */
    bool Success;

    /**
     * @brief The process's working directory path. Valid only when
     *        `Success` is `true`.
     */
    char Path[MaxWorkingDirectoryLength];
  };
}
