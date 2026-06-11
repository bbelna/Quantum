/**
 * @file Servers/App/Dock/DockConstants.hpp
 * @brief Dock subsystem constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

namespace Quantum::Servers::App::Dock {
  static constexpr bool BottomBezel = false;
  static constexpr bool ButtonsOverlapBezel = false;
  static constexpr UInt16 BezelThickness = 0;
  static constexpr UInt16 Padding = 0;
  static constexpr UInt16 ButtonFallbackHeight = 20;
  static constexpr UInt16 ButtonVerticalPadding = 6;
  static constexpr UInt16 ButtonDefaultWidth = 0;
  static constexpr UInt16 ButtonMinWidth = 48;
  static constexpr UInt16 ButtonSpacing = 0;
  static constexpr bool ButtonSeparator = false;
  static constexpr UInt16 ButtonSeparatorWidth = 1;
  static constexpr UInt16 TextPadding = 12;
  static constexpr Size MaxButtons = 64;

  /**
   * @brief Thickness of the 1px chrome border drawn along the top
   *        edge of the dock in @ref Theme::WindowBorderColor. The
   *        maximized window stretches down by this many pixels so
   *        its own bottom border overlaps the same row.
   */
  static constexpr UInt16 TopBorder = 1;
}
