/**
 * @file Include/Quantum/HAL/PCI/Payloads/EnableBusMasteringPayload.hpp
 * @brief Declares @ref Quantum::HAL::PCI::Payloads::EnableBusMasteringPayload.
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
   * @brief Payload for @ref PCIDriverOperation::EnableBusMastering.
   *
   * Sets bit 2 (Bus Master Enable) in the PCI Command register (offset
   * 0x04) for the specified device, allowing it to initiate DMA transfers
   * on the PCI bus.
   */
  struct EnableBusMasteringPayload {
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
  };
}
