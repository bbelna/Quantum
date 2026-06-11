/**
 * @file Include/Quantum/HAL/Graphics/Payloads/DisplayOwnerPayload.hpp
 * @brief Declaration of the DisplayOwnerPayload structure.
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
   * @brief Payload for the `GetDisplayOwner` graphics driver operation.
   *
   * Filled by the driver to report the current display owner. If no process
   * owns the display (text mode), `OwnerPID` is 0 and `IsCompositing` is
   * false.
   */
  struct DisplayOwnerPayload {
    /**
     * @brief @ref ProcessID of the current display owner, or 0 if none.
     */
    UInt32 OwnerPID;

    /**
     * @brief `true` if the display is currently in compositing mode.
     */
    bool IsCompositing;
  };
}
