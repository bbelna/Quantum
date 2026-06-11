/**
 * @file Include/Quantum/HAL/PCI/Payloads/WriteConfigPayload.hpp
 * @brief Declares @ref Quantum::HAL::PCI::Payloads::WriteConfigPayload.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::PCI::Payloads {
  /**
   * @brief Payload for @ref PCIDriverOperation::WriteConfig.
   */
  struct WriteConfigPayload {
    /**
     * @brief PCI bus number.
     */
    UInt8 Bus;

    /**
     * @brief Device slot number.
     */
    UInt8 Slot;

    /**
     * @brief Function number.
     */
    UInt8 Function;

    /**
     * @brief Dword-aligned register offset (0x00-0xFC).
     */
    UInt8 Offset;

    /**
     * @brief The 32-bit value to write to configuration space.
     */
    UInt32 Value;
  };
}
