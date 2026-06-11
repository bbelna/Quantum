/**
 * @file Servers/App/Windows/WindowTitleBarTheme.hpp
 * @brief Declares @ref @QAppSrv::Windows::TrafficLightButtonTheme and
 *        @ref @QAppSrv::Windows::WindowTitleBarTheme.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Theming information for traffic-light window buttons.
   */
  struct TrafficLightButtonTheme {
    UInt32 CloseColor;
    UInt32 ClosePressedColor;
    UInt32 MaximizeColor;
    UInt32 MaximizePressedColor;
    UInt32 MinimizeColor;
    UInt32 MinimizePressedColor;
    UInt32 InactiveColor;
  };

  /**
   * @brief Theming information for a window title bar.
   *
   * The title bar height is computed from the @ref Padding plus the
   * tallest interior content (button bitmaps or font glyphs) rather
   * than stored as a fixed value.
   */
  struct WindowTitleBarTheme {
    UInt32 ActiveBackground;
    UInt32 InactiveBackground;
    UInt32 ActiveTextColor;
    UInt32 InactiveTextColor;
    UInt32 SeparatorColor;
    bool EnableBottomBorder;
    bool CenterTitle;
    Inset Padding;
    TrafficLightButtonTheme Buttons;
  };
}
