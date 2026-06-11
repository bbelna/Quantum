/**
 * @file Include/Quantum/HAL/PCI/Payloads/ReadConfigPayload.hpp
 * @brief Declares @ref Quantum::HAL::PCI::Payloads::ReadConfigPayload.
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
   * @brief In-out payload for @ref PCIDriverOperation::ReadConfig.
   *
   * The caller fills in the address fields; the driver fills in @ref Value.
   */
  struct ReadConfigPayload {
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
     * @brief [out] The 32-bit value read from configuration space.
     */
    UInt32 Value;
  };
}
