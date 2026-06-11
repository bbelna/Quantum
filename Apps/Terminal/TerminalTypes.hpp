/**
 * @file Apps/Terminal/TerminalTypes.hpp
 * @brief Declares @ref @QApps::Terminal types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/App.hpp>
#include <Quantum/Core.hpp>
#include <Quantum/Fonts.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/UI.hpp>

namespace Quantum::Apps::Terminal {
  class Terminal;
}

using namespace Quantum;
using namespace Quantum::App;
using namespace Quantum::Apps::Terminal;
using namespace Quantum::Clients;
using namespace Quantum::Core;
using namespace Quantum::Components;
using namespace Quantum::Fonts;
using namespace Quantum::Input;
using namespace Quantum::Kernel::Memory;
using namespace Quantum::Geometry2D;
using namespace Quantum::Menus;
using namespace Quantum::Servers::FileSystem::ABI;
using namespace Quantum::Threading;
using namespace Quantum::UI;
using namespace Quantum::Streaming;
