/**
 * @file Servers/App/Windows/DefaultWindowTheme.hpp
 * @brief Provides a factory for the default @ref WindowTheme.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "WindowTheme.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Returns the default @ref WindowTheme.
   *
   * All values are defined directly as literals — this is the single
   * source of truth for the default window appearance.
   */
  inline WindowTheme DefaultWindowTheme() {
    return WindowTheme {
      .Background = 0xFF404040,
      .ContentPadding = 0,

      .TitleBar = {
        .ActiveBackground = 0xFF000000,
        .InactiveBackground = 0xFF101010,
        .ActiveTextColor = 0xFFFFFFFF,
        .InactiveTextColor = 0xFFA0A0A0,
        .SeparatorColor = 0xFF303030,
        .EnableBottomBorder = false,
        .CenterTitle = false,
        .Padding = { 7, 7, 7, 6 },

        .Buttons = {
          .CloseColor = 0xFFFF5F57,
          .ClosePressedColor = 0xFFCC4C46,
          .MaximizeColor = 0xFF28C840,
          .MaximizePressedColor = 0xFF20A033,
          .MinimizeColor = 0xFFFEBB2E,
          .MinimizePressedColor = 0xFFCB9625,
          .InactiveColor = 0xFF707070,
        },
      },

      .Border = {
        .Color = 0xFF404040,
        .Thickness = 1,
        .EnableOuterBorder = true,
        .OuterBorderColor = 0xFF000000,
        .InnerColor = 0xFF303030,
        .InnerThickness = 1,
      },

      .Bezel = {
        .LightColor = 0xFF606060,
        .DarkColor = 0xFF202020,
        .OuterDarkColor = 0xFF000000,
        .Thickness = 1,
      },

      .ResizeGrip = {
        .DotColor = 0xFF888888,
        .DotSize = 2,
        .DotGap = 4,
        .HitSize = 12,
      },
    };
  }
}
