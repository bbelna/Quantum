/**
 * @file Include/Quantum/HAL/Graphics/Payloads/SetHardwareCursorVisiblePayload.hpp
 * @brief Declaration of the SetHardwareCursorVisiblePayload structure.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::HAL::Graphics::Payloads {
  /**
   * @brief Payload for the `SetHardwareCursorVisible` operation.
   */
  struct SetHardwareCursorVisiblePayload {
    /**
     * @brief `true` to show the cursor, false to hide it.
     */
    bool Visible;
  };
}
