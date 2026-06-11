/**
 * @file Include/Quantum/HAL/Graphics/Payloads/ReleaseDisplayPayload.hpp
 * @brief Declaration of the ReleaseDisplayPayload structure.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::Graphics::Payloads {
  /**
   * @brief Payload for the `ReleaseDisplay` graphics driver operation.
   *
   * The caller supplies its `OwnerPID`. If the PID matches the current
   * display owner, the driver transitions back to text mode.
   */
  struct ReleaseDisplayPayload {
    /**
     * @brief @ref ProcessID of the process releasing display ownership.
     */
    UInt32 OwnerPID;
  };
}
