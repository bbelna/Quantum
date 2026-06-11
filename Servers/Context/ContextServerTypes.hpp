/**
 * @file Servers/Context/ContextServerTypes.hpp
 * @brief Declares core @ref @QCtxSrv types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Fonts.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/Clients.hpp>
#include <Quantum/Servers/Core.hpp>
#include <Quantum/Threading.hpp>
#include <Quantum/UI.hpp>

using namespace Quantum::Clients;
using namespace Quantum::Components;
using namespace Quantum::Core;
using namespace Quantum::Fonts;
using namespace Quantum::Geometry2D;
using namespace Quantum::Threading;
using namespace Quantum::Menus;
using namespace Quantum::Servers::Core;
using namespace Quantum::Servers::Context;
using namespace Quantum::UI;

namespace CtxABI = Quantum::Servers::Context::ABI;
namespace FS = Quantum::Servers::FileSystem::ABI;
namespace AppServer = Quantum::Servers::App::ABI;
