/**
 * @file Include/Quantum/Clients.hpp
 * @brief Umbrella header for Quantum OS client libraries.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Clients/ContextClient.hpp"
#include "Clients/DeviceClient.hpp"
#include "Clients/DriverClient.hpp"
#include "Clients/DisplayClient.hpp"
#include "Clients/FileSystemClient.hpp"
#include "Clients/GraphicsClient.hpp"
#include "Clients/GraphicsDriverClient.hpp"
#include "Clients/InputClient.hpp"
#include "Clients/KernelClient.hpp"
#include "Clients/PCIDriverClient.hpp"
#include "Clients/RunClient.hpp"
#include "Clients/StartupClient.hpp"
#include "Clients/StreamClient.hpp"

/**
 * @brief Client-side interfaces to QuantumOS servers.
 */
namespace Quantum::Clients {}
