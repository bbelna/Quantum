/**
 * @file Servers/App/Dock/DockButton.hpp
 * @brief Declares @ref @QAppSrv::Dock::DockButton.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

namespace Quantum::Servers::App::Dock {
  /**
   * @brief A single button in the dock taskbar, representing one window.
   */
  struct DockButton {
    UInt32 WindowResourceID = 0;
    Int16 X = 0;
    UInt16 Width = 0;
    bool Focused = false;
    bool Minimized = false;
    char Title[32] = {};
  };
}
