/**
 * @file Include/Quantum/FileSystems/QFS/QFSVolumeState.hpp
 * @brief Declares @ref QFSVolumeState.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::QFS {
  /**
   * @brief Volume mount state values.
   */
  enum class QFSVolumeState : UInt32 {
    /**
     * @brief Volume was cleanly unmounted.
     */
    Clean = 0,

    /**
     * @brief Volume is currently mounted (or was not cleanly unmounted).
     */
    Mounted = 1,

    /**
     * @brief Volume has errors and needs repair.
     */
    Errors = 2
  };
}
