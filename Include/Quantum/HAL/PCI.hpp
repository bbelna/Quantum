/**
 * @file Include/Quantum/HAL/PCI.hpp
 * @brief Aggregates the PCI bus driver interface and all associated payload
 *        structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "PCI/PCIDriverOperation.hpp"
#include "PCI/PCIDeviceInfo.hpp"
#include "PCI/Payloads.hpp"

/**
 * @brief PCI bus driver interfaces.
 *
 * This namespace provides the @ref PCIDriverOperation dispatch codes, the
 * shared @ref PCIDeviceInfo structure, and the
 * @ref Quantum::HAL::PCI::Payloads sub-namespace containing the
 * operation-specific payload structures used with `IDriver::Invoke`.
 */
namespace Quantum::HAL::PCI {}
