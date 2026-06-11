/**
 * @file Include/Quantum/HAL/Graphics/Payloads/FramebufferBufferIDPayload.hpp
 * @brief Declaration of the FramebufferBufferIDPayload structure.
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
   * @brief Payload for the `GetFramebufferBufferID` query. Filled by the
   *        driver; the caller allocates and zeroes the struct before calling.
   */
  struct FramebufferBufferIDPayload {
    /**
     * @brief The SharedBufferID of the VRAM framebuffer, or 0 if direct
     *        VRAM access is not available.
     */
    UInt32 BufferID;
  };
}
