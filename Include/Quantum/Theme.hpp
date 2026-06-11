/**
 * @file Include/Quantum/Theme.hpp
 * @brief Centralized UI color and dimension constants for the composited
 *        window appearance.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Theme {
  /**
   * @brief Whether dark mode is active.
   *
   * When `true`, UI elements that carry a light-mode default palette
   * (such as the system menu logo) invert their colors automatically.
   */
  static constexpr bool DarkMode = true;

  /**
   * @brief Whether translucency effects are enabled.
   *
   * When `false`, all compositing paths treat pixels as fully opaque
   * regardless of their alpha channel, avoiding the per-pixel blending
   * cost.
   */
  static constexpr bool EnableTranslucency = true;

  /**
   * @brief Whether drop shadows are drawn around windows and menus.
   *
   * When `false`, shadow passes are skipped entirely, saving the
   * per-pixel alpha blending cost of the soft shadow layers.
   */
  static constexpr bool EnableShadows = false;

  /**
   * @brief Whether the cursor shadow is drawn.
   */
  static constexpr bool EnableCursorShadow = false;

  /**
   * @brief Whether window border corners are anti-aliased.
   *
   * When `false`, corners use hard-edged pixel coverage instead of
   * sub-pixel alpha blending.
   */
  static constexpr bool EnableBorderCornerAA = false;

  /**
   * @brief Desktop background color.
   */
  static constexpr UInt32 DesktopBackground = 0xFF202020;

  /**
   * @brief Default window chrome and content area background color.
   */
  static constexpr UInt32 WindowBackground = 0xFF303030;

  /**
   * @brief 1px outline color for the window border. Also used for the
   *        title bar's bottom separator so the chrome outline is
   *        consistent on all four sides.
   */
  static constexpr UInt32 WindowBorderColor = 0xFF000000;

  /**
   * @brief Whether to draw a second 1px outline on the outside of the
   *        normal window border. When enabled, everything inside the
   *        chrome (the normal border, the padding band, the title bar,
   *        and the content area) shifts inward by 1 pixel on all four
   *        sides to make room for this outer stroke.
   */
  static constexpr bool EnableWindowOuterBorder = true;

  /**
   * @brief Color of the optional outer window border stroke. Only used
   *        when `EnableWindowOuterBorder` is `true`.
   */
  static constexpr UInt32 WindowOuterBorderColor = 0xFF000000;

  /**
   * @brief Shadow color.
   */
  static constexpr UInt32 Shadow = 0xFF000000;

  /**
   * @brief Corner radius for window chrome.
   */
  static constexpr UInt8 WindowCornerRadius = 0;

  /**
   * @brief Number of pixels of soft drop shadow (spread).
   * @note Applied uniformly to windows and dropdown menus.
   */
  static constexpr UInt8 ShadowSize = 6;

  /**
   * @brief Peak alpha of the innermost shadow layer.
   */
  static constexpr UInt8 ShadowAlpha = 0x10;

  /**
   * @brief Global horizontal shadow offset.
   * @note Applied to both window and dropdown shadows.
   */
  static constexpr Int8 ShadowOffsetX = 0;

  /**
   * @brief Global vertical shadow offset (positive = down).
   */
  static constexpr Int8 ShadowOffsetY = 0;

  /**
   * @brief Active (focused) title bar background color.
   */
  static constexpr UInt32 TitleBarActive = 0xFF000000;

  /**
   * @brief Inactive (unfocused) title bar background color.
   */
  static constexpr UInt32 TitleBarInactive = 0xFF101010;

  /**
   * @brief 1px separator between title bar and content area.
   */
  static constexpr UInt32 TitleBarSeparator = 0xFF303030;

  /**
   * @brief Whether to draw the 1px border along the bottom edge of the
   *        window title bar (the separator between the title bar and
   *        the content area). When `false`, the title bar fill extends
   *        flush into the content area with no dividing line.
   */
  static constexpr bool EnableTitleBarBottomBorder = false;

  /**
   * @brief Active title bar text color.
   */
  static constexpr UInt32 TitleBarTextActive = 0xFFFFFFFF;

  /**
   * @brief Inactive title bar text color.
   */
  static constexpr UInt32 TitleBarTextInactive = 0xFFA0A0A0;

  /**
   * @brief Whether window title bar text is centered horizontally
   *        within the title bar chrome.
   */
  static constexpr bool CenterWindowTitle = false;

  /**
   * @brief Close button color.
   */
  static constexpr UInt32 TrafficLightClose = 0xFFFF5F57;

  /**
   * @brief Close button pressed color.
   */
  static constexpr UInt32 TrafficLightClosePressed = 0xFFCC4C46;

  /**
   * @brief Minimize button color.
   */
  static constexpr UInt32 TrafficLightMinimize = 0xFFFEBB2E;

  /**
   * @brief Minimize button pressed color.
   */
  static constexpr UInt32 TrafficLightMinimizePressed = 0xFFCB9625;

  /**
   * @brief Maximize button color.
   */
  static constexpr UInt32 TrafficLightMaximize = 0xFF28C840;

  /**
   * @brief Maximize button pressed color.
   */
  static constexpr UInt32 TrafficLightMaximizePressed = 0xFF20A033;

  /**
   * @brief Inactive light button color.
   */
  static constexpr UInt32 TrafficLightInactive = 0xFF707070;

  /**
   * @brief Light bezel color (top/left highlight edge).
   */
  static constexpr UInt32 BezelLight = 0xFF555555;

  /**
   * @brief Dark bezel color (bottom/right shadow edge).
   */
  static constexpr UInt32 BezelDark = 0xFF222222;

  /**
   * @brief Outer dark bezel color (outermost shadow).
   */
  static constexpr UInt32 BezelOuterDark = 0xFF111111;

  /**
   * @brief Button face / background color.
   */
  static constexpr UInt32 ButtonFace = 0xFF404040;

  /**
   * @brief Button glyph / foreground color.
   */
  static constexpr UInt32 ButtonGlyph = 0xFFEEEEEE;

  /**
   * @brief Menu bar background color.
   */
  static constexpr UInt32 MenuBarBackground = 0xFF101010;

  /**
   * @brief Active (open) menu title button background color. Fills the
   *        title's button area when its dropdown is open, providing a
   *        visual pill behind the title text/logo. The bottom
   *        highlight stripe overlays this fill along the menu bar's
   *        chrome border row.
   */
  static constexpr UInt32 MenuBarActiveBackground = 0xFF000000;

  /**
   * @brief Menu bar text color.
   */
  static constexpr UInt32 MenuBarText = 0xFFFFFFFF;

  /**
   * @brief Menu bar text color for inactive titles and the system
   *        menu icon when no dropdown for that title is open.
   */
  static constexpr UInt32 MenuBarTextInactive = 0xFFB0B0B0;

  /**
   * @brief Dropdown panel background color.
   */
  static constexpr UInt32 DropdownBackground = 0xFF101010;

  /**
   * @brief Dropdown inner bezel highlight (softer than pure white).
   */
  static constexpr UInt32 DropdownBezelInnerLight = 0xFF444444;

  /**
   * @brief Highlight color for hovered/selected menu items.
   */
  static constexpr UInt32 HighlightColor = 0xFF3060C0;

  static constexpr UInt32 HighlightColor2 = 0xFF000000;

  static constexpr UInt32 TextForeground = 0xFFFFFFFF;

  static constexpr UInt32 TextBackground = WindowBackground;

  /**
   * @brief Text color on highlighted menu items.
   */
  static constexpr UInt32 HighlightText = 0xFFFFFFFF;

  /**
   * @brief Disabled / grayed-out text color.
   */
  static constexpr UInt32 DisabledText = 0xFF666666;

  /**
   * @brief Separator line color in dropdown menus.
   */
  static constexpr UInt32 SeparatorColor = WindowBorderColor;

  /**
   * @brief Pure black, used for glyphs and outlines.
   */
  static constexpr UInt32 Black = 0xFF000000;

  /**
   * @brief Border radius for dropdown menus (bottom corners only).
   */
  static constexpr UInt8 DropdownBorderRadius = 0;

  /**
   * @brief Corner radius for push buttons.
   */
  static constexpr UInt8 ButtonCornerRadius = 0;

  /**
   * @brief Border color for push buttons (normal state).
   */
  static constexpr UInt32 ButtonBorderColor = WindowBorderColor;

  /**
   * @brief Pressed button face color.
   */
  static constexpr UInt32 ButtonPressedFace = 0xFF101010;

  /**
   * @brief Corner radius for text input fields.
   */
  static constexpr UInt8 TextInputCornerRadius = 0;

  /**
   * @brief Border color for text input fields (normal state).
   */
  static constexpr UInt32 TextInputBorderColor = 0xFF808080;

  /**
   * @brief Border color for focused text input fields.
   */
  static constexpr UInt32 TextInputFocusBorderColor = 0xFF3366CC;

  /**
   * @brief Number of pixels of shadow spread around the cursor.
   */
  static constexpr UInt8 CursorShadowSize = 2;

  /**
   * @brief Peak alpha of the innermost cursor shadow layer.
   *
   * Higher than the window shadow because the cursor is small and
   * the shadow must be visible at a glance.
   */
  static constexpr UInt8 CursorShadowAlpha = 0x28;

  /**
   * @brief Horizontal offset of the cursor shadow (positive = right).
   */
  static constexpr Int8 CursorShadowOffsetX = 1;

  /**
   * @brief Vertical offset of the cursor shadow (positive = down).
   */
  static constexpr Int8 CursorShadowOffsetY = 1;

  /**
   * @brief Resize grip dot color.
   */
  static constexpr UInt32 ResizeGripDot = 0xFF888888;

  /**
   * @brief Dock background color.
   */
  static constexpr UInt32 DockBackground = 0xFF101010;

  /**
   * @brief Corner radius for dock buttons. Set to 0 for square buttons.
   */
  static constexpr UInt8 DockButtonBorderRadius = 0;

  /**
   * @brief Whether to round the corners of dock buttons.
   */
  static constexpr bool DockButtonRoundedCorners = false;

  /**
   * @brief Whether to draw a highlight strip along the top edge of
   *        the active dock button. The strip is drawn over the dock's
   *        1px chrome border row so the active button visually
   *        replaces the chrome line within its width.
   */
  static constexpr bool DockButtonTopHighlight = true;

  /**
   * @brief Color of the active dock button's top highlight strip.
   */
  static constexpr UInt32 DockButtonTopHighlightColor = 0xFF3366CC;

  /**
   * @brief Thickness of the active dock button's top highlight strip
   *        in pixels (includes the chrome border row it overlays).
   */
  static constexpr UInt8 DockButtonTopHighlightThickness = 2;

  /**
   * @brief Default background color for dock buttons.
   */
  static constexpr UInt32 DockButtonDefaultBackground = MenuBarBackground;

  /**
   * @brief Active (focused) dock button background color.
   */
  static constexpr UInt32 DockButtonActiveBackground = MenuBarActiveBackground;

  /**
   * @brief Whether to draw a border outline around the active dock
   *        button.
   */
  static constexpr bool DockButtonActiveBorderEnabled = false;

  /**
   * @brief Border color for the active (focused) dock button.
   */
  static constexpr UInt32 DockButtonActiveBorder = 0xFF666666;

  /**
   * @brief Active (focused) dock button text color.
   */
  static constexpr UInt32 DockButtonActiveText = 0xFFFFFFFF;

  /**
   * @brief Inactive (unfocused) dock button text color.
   */
  static constexpr UInt32 DockButtonInactiveText = 0xFFB0B0B0;

  /**
   * @brief Dock button separator color.
   */
  static constexpr UInt32 DockButtonSeparator = 0x77AAAAAA;
}
