/**
 * @file Include/Quantum/HAL/Graphics/Payloads/FastScreenBlitSupportPayload.hpp
 * @brief Declaration of the FastScreenBlitSupportPayload structure.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::HAL::Graphics::Payloads {
  /**
   * @brief Payload for the `GetFastScreenBlitSupport` query. Filled by the
   *        driver; the caller allocates and zeroes the struct before calling.
   */
  struct FastScreenBlitSupportPayload {
    /**
     * @brief Set to `true` by the driver if hardware-accelerated
     *        screen-to-screen BLT is available.
     */
    bool Supported;
  };
}
