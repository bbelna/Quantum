/**
 * @file Include/Quantum/HAL/PCI/Payloads.hpp
 * @brief Aggregates all PCI driver operation payload structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Payloads/ReadConfigPayload.hpp"
#include "Payloads/WriteConfigPayload.hpp"
#include "Payloads/FindDeviceByIDPayload.hpp"
#include "Payloads/FindDeviceByClassPayload.hpp"
#include "Payloads/ReadBARPayload.hpp"
#include "Payloads/EnableBusMasteringPayload.hpp"

/**
 * @brief Operation-specific payload structures for the PCI bus driver
 *        interface.
 *
 * Each struct in this namespace corresponds to a @ref PCIDriverOperation
 * code and carries the typed arguments for that operation through the
 * generic `IDriver::Invoke` dispatch path.
 */
namespace Quantum::HAL::PCI::Payloads {}
