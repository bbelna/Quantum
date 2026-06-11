/**
 * @file Servers/App/Windows/WindowTheme.hpp
 * @brief Declares the complete @ref @QAppSrv::Windows::WindowTheme struct
 *        hierarchy.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "WindowTitleBarTheme.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Theming information for a window's border.
   */
  struct BorderTheme {
    UInt32 Color;
    UInt8 Thickness;
    bool EnableOuterBorder;
    UInt32 OuterBorderColor;
    UInt32 InnerColor;
    UInt8 InnerThickness;
  };

  /**
   * @brief Theming information for a 3D beveled bezel edge.
   *
   * When @ref Thickness is 0 the bezel is disabled and no pixels are
   * drawn.
   */
  struct BezelTheme {
    UInt32 LightColor;
    UInt32 DarkColor;
    UInt32 OuterDarkColor;
    UInt8 Thickness;
  };

  /**
   * @brief Theming information for a window's resize grip.
   */
  struct ResizeGripTheme {
    UInt32 DotColor;
    UInt16 DotSize;
    UInt16 DotGap;
    UInt16 HitSize;
  };

  /**
   * @brief Complete theming information for a composited window.
   *
   * Aggregates all sub-themes (title bar, border, bezel, resize grip)
   * into a single value type that is passed by reference to
   * @ref Window construction.
   */
  struct WindowTheme {
    UInt32 Background;
    UInt16 ContentPadding;
    WindowTitleBarTheme TitleBar;
    BorderTheme Border;
    BezelTheme Bezel;
    ResizeGripTheme ResizeGrip;
  };
}
