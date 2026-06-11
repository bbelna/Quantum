/**
 * @file Include/Quantum/Servers/Run/ABI/LoadELFResult.hpp
 * @brief Declares @ref LoadELFResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Threading.hpp>

namespace Quantum::Servers::Run::ABI {
  /**
   * @brief Result of a @ref RunServerOperation::LoadELF request.
   */
  struct LoadELFResult {
    /**
     * @brief `true` if the ELF was loaded successfully; `false` otherwise.
     */
    bool Success;

    /**
     * @brief The process ID of the spawned process. Valid only when
     *        `Success` is `true`.
     */
    ProcessID PID;
  };
}
