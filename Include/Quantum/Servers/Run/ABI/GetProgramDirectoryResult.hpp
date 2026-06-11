/**
 * @file Include/Quantum/Servers/Run/ABI/GetProgramDirectoryResult.hpp
 * @brief Declares @ref GetProgramDirectoryResult.
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
   * @brief Result of a @ref RunServerOperation::GetProgramDirectory request.
   */
  struct GetProgramDirectoryResult {
    /**
     * @brief `true` if the process was found; `false` otherwise.
     */
    bool Success;

    /**
     * @brief The process's program directory path. Valid only when
     *        `Success` is `true`.
     */
    char Path[MaxWorkingDirectoryLength];
  };
}
