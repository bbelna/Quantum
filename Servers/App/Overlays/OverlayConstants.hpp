/**
 * @file Servers/App/Overlays/OverlayConstants.hpp
 * @brief Overlay subsystem constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

namespace Quantum::Servers::App::Overlays {
  /**
   * @brief Maximum number of simultaneous overlays.
   */
  static constexpr Size MaxOverlays = 16;
}
