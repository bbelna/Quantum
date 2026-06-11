/**
 * @file Include/Quantum/HAL/Graphics/Payloads/AcquireDisplayPayload.hpp
 * @brief Declaration of the AcquireDisplayPayload structure.
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
   * @brief Payload for the `AcquireDisplay` graphics driver operation.
   *
   * The caller supplies its `OwnerPID`. On success, the driver transitions
   * to compositing mode and all subsequent text-mode operations become
   * no-ops until `ReleaseDisplay` is called.
   */
  struct AcquireDisplayPayload {
    /**
     * @brief @ref ProcessID of the process acquiring display ownership.
     */
    UInt32 OwnerPID;
  };
}
