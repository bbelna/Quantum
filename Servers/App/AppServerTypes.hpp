/**
 * @file Servers/App/AppServerTypes.hpp
 * @brief Declares core @ref @QAppSrv types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Cursors.hpp>
#include <Quantum/Fonts.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/Input.hpp>
#include <Quantum/OS.hpp>
#include <Quantum/Runtime.hpp>
#include <Quantum/Structures.hpp>
#include <Quantum/Sync.hpp>
#include <Quantum/Threading.hpp>
#include <Quantum/UI.hpp>
#include <Quantum/Servers/Core.hpp>

#include "Core/DrawContext.hpp"

namespace Quantum::Servers::App {
  namespace Theme = Quantum::Theme;

  class Compositor;

  namespace Controllers {
    class OverlayController;
    class WindowController;
  }

  namespace Cursor {
    class CursorManager;
  }

  namespace Dock {
    struct DockButton;

    class Dock;
  }

  namespace Overlays {
    struct Overlay;

    class OverlayManager;
  }

  namespace Windows {
    class WindowManager;
  }
}

using namespace Quantum::Clients;
using namespace Quantum::Core;
using namespace Quantum::Cursors;
using namespace Quantum::Fonts;
using namespace Quantum::Geometry2D;
using namespace Quantum::Input;
using namespace Quantum::Kernel;
using namespace Quantum::Kernel::IPC;
using namespace Quantum::Servers::Core;
using namespace Quantum::Servers::App;
using namespace Quantum::Servers::App::Controllers;
using namespace Quantum::Servers::App::Windows;
using namespace Quantum::Servers::App::Cursor;
using namespace Quantum::Servers::App::Overlays;
using namespace Quantum::Servers::Graphics::ABI;
using namespace Quantum::Structures::Lists;
using namespace Quantum::Structures::Nodes;
using namespace Quantum::Sync;
using namespace Quantum::Threading;
using namespace Quantum::UI;
using namespace Quantum::Menus;
using namespace Quantum::Components;
