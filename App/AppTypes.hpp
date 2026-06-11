/**
 * @file App/AppTypes.hpp
 * @brief Declares core @ref @QApp types.
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
#include <Quantum/UI.hpp>

namespace Quantum::App {
  namespace AppServer = Quantum::Servers::App::ABI;
  namespace Memory = Quantum::Kernel::Memory;
}

using namespace Quantum::App;
using namespace Quantum::Core;
using namespace Quantum::Components;
using namespace Quantum::Fonts;
using namespace Quantum::Geometry2D;
using namespace Quantum::UI;
using namespace Quantum::Menus;
