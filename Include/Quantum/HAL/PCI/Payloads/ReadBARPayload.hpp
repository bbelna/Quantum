/**
 * @file Include/Quantum/HAL/PCI/Payloads/ReadBARPayload.hpp
 * @brief Declares @ref Quantum::HAL::PCI::Payloads::ReadBARPayload.
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
   * @brief In-out payload for @ref PCIDriverOperation::ReadBAR.
   *
   * The caller fills in the address fields and @ref BARIndex; the driver
   * fills in @ref Value.
   */
  struct ReadBARPayload {
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
     * @brief BAR index (0-5).
     */
    UInt8 BARIndex;

    /**
     * @brief [out] The raw 32-bit BAR value. For I/O BARs, mask with
     *        `0xFFFFFFFC`; for memory BARs, mask with `0xFFFFFFF0`.
     */
    UInt32 Value;
  };
}
