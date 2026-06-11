/**
 * @file Include/Quantum/HAL/PCI/PCIDriverOperation.hpp
 * @brief Declares @ref Quantum::HAL::PCI::PCIDriverOperation.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::PCI {
  /**
   * @brief Operation codes for the PCI bus driver.
   */
  enum class PCIDriverOperation : UInt32 {
    /**
     * @brief Reads a 32-bit value from PCI configuration space.
     *
     * Payload: pointer to a @ref Payloads::ReadConfigPayload (in-out).
     * Returns 1 on success, 0 on failure.
     */
    ReadConfig = 1,

    /**
     * @brief Writes a 32-bit value to PCI configuration space.
     *
     * Payload: pointer to a @ref Payloads::WriteConfigPayload.
     * Returns 1 on success, 0 on failure.
     */
    WriteConfig = 2,

    /**
     * @brief Finds a PCI device by vendor and device ID.
     *
     * Payload: pointer to a @ref Payloads::FindDeviceByIDPayload (in-out).
     * Returns 1 if found, 0 if not found.
     */
    FindDeviceByID = 3,

    /**
     * @brief Finds a PCI device by class code (class + subclass).
     *
     * Payload: pointer to a @ref Payloads::FindDeviceByClassPayload (in-out).
     * Returns 1 if found, 0 if not found.
     */
    FindDeviceByClass = 4,

    /**
     * @brief Reads a Base Address Register (BAR) from a PCI device.
     *
     * Payload: pointer to a @ref Payloads::ReadBARPayload (in-out).
     * Returns 1 on success, 0 on failure.
     */
    ReadBAR = 5,

    /**
     * @brief Enables PCI bus mastering for a device by setting bit 2 of the
     *        PCI Command register.
     *
     * Payload: pointer to a @ref Payloads::EnableBusMasteringPayload.
     * Returns 1 on success, 0 on failure.
     */
    EnableBusMastering = 6
  };
}
