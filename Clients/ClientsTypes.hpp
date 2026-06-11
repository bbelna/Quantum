/**
 * @file Clients/ClientsTypes.hpp
 * @brief Declares core @ref @QClients types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/Memory.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/Runtime.hpp>
#include <Quantum/UI.hpp>

using namespace Quantum::Clients;
using namespace Quantum::Core;
using namespace Quantum::Components;
using namespace Quantum::Geometry2D;
using namespace Quantum::HAL;
using namespace Quantum::HAL::Graphics;
using namespace Quantum::HAL::Graphics::Payloads;
using namespace Quantum::Kernel;
using namespace Quantum::Kernel::ABI;
using namespace Quantum::Kernel::Concurrency;
using namespace Quantum::Kernel::IPC;
using namespace Quantum::Kernel::Memory;
using namespace Quantum::Kernel::Resources;
using namespace Quantum::Menus;
using namespace Quantum::Servers::Context::ABI;
using namespace Quantum::Servers::Device::ABI;
using namespace Quantum::Servers::FileSystem::ABI;
using namespace Quantum::Servers::Graphics::ABI;
using namespace Quantum::Servers::Input::ABI;
using namespace Quantum::Servers::Run;
using namespace Quantum::Servers::Run::ABI;
using namespace Quantum::Servers::Startup::ABI;
using namespace Quantum::Servers::Stream;
using namespace Quantum::Structures::Lists;
using namespace Quantum::Threading;
using namespace Quantum::UI;
