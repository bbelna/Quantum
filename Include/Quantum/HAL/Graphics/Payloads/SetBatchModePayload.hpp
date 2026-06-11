/**
 * @file Include/Quantum/HAL/Graphics/Payloads/SetBatchModePayload.hpp
 * @brief Declaration of the SetBatchModePayload structure.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::HAL::Graphics::Payloads {
  /**
   * @brief Payload for the `SetBatchMode` graphics driver operation.
   */
  struct SetBatchModePayload {
    /**
     * @brief `true` to enable batch mode (shadow-only writes),
     *        false to disable.
     */
    bool Enabled;
  };
}
