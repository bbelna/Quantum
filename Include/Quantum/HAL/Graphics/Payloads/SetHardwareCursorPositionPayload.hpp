/**
 * @file Include/Quantum/HAL/Graphics/Payloads/SetHardwareCursorPositionPayload.hpp
 * @brief Declaration of the SetHardwareCursorPositionPayload structure.
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
   * @brief Payload for the `SetHardwareCursorPosition` operation.
   */
  struct SetHardwareCursorPositionPayload {
    /**
     * @brief New X screen coordinate (clamped to [0, screen width]).
     */
    Int16 X;

    /**
     * @brief New Y screen coordinate (clamped to [0, screen height]).
     */
    Int16 Y;
  };
}
