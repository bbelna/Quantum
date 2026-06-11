/**
 * @file Servers/Device/DeviceServerTypes.hpp
 * @brief Declares core @ref @QDvSrv types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Clients.hpp>
#include <Quantum/HAL.hpp>
#include <Quantum/Servers/Core.hpp>
#include <Quantum/Servers/Device/ABI.hpp>
#include <Quantum/Structures.hpp>

namespace Quantum::Servers::Device {
  // `Device` is also a namespace name in `Quantum::Servers`, so the
  // alias must be declared inside this namespace to shadow it for code
  // that looks up `Device` from within @ref @QDvSrv.
  using Device = Quantum::HAL::Device;
}

using namespace Quantum::Core;

using namespace Quantum::Clients;
using namespace Quantum::HAL;
using namespace Quantum::Servers::Core;
using namespace Quantum::Servers::Device;
using namespace Quantum::Servers::Device::ABI;

using namespace Quantum::Structures::Lists;
