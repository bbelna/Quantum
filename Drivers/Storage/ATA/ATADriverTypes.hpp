/**
 * @file Drivers/Storage/ATA/ATADriverTypes.hpp
 * @brief Declares core @ref @QDrvs::Storage::ATA types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/Servers/Core.hpp>
#include <Quantum/Threading.hpp>

namespace Quantum::Drivers::Storage::ATA {
  namespace StorageABI = Quantum::Servers::Storage::ABI;
  namespace DriverABI = Quantum::Servers::Storage::DriverABI;
}

using namespace Quantum::Core;
using namespace Quantum::Threading;

using namespace Quantum::Clients;
using namespace Quantum::HAL;
using namespace Quantum::Kernel;
using namespace Quantum::Servers::Core;
using namespace Quantum::Drivers::Storage::ATA;
