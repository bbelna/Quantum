/**
 * @file Include/Quantum/HAL/Graphics/Payloads/HardwareCursorSupportPayload.hpp
 * @brief Declaration of the HardwareCursorSupportPayload structure.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::HAL::Graphics::Payloads {
  /**
   * @brief Payload for the `GetHardwareCursorSupport` query. Filled by the
   *        driver; the caller allocates and zeroes the struct before calling.
   */
  struct HardwareCursorSupportPayload {
    /**
     * @brief Set to `true` by the driver if a hardware cursor is available.
     */
    bool Supported;
  };
}
