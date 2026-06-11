/**
 * @file Servers/Context/MenuBar.hpp
 * @brief Declares @ref @QCtxSrv::MenuBar.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once
#include <Quantum/Fonts/BitmapFont.hpp>
#include <Quantum/Theme.hpp>
#include <Quantum/UI/Canvas.hpp>

#include "ContextServerTypes.hpp"

namespace Quantum::Servers::Context {
  /**
   * @brief Vertical padding above and below the menu text in the bar.
   */
  static constexpr UInt16 MenuBarVerticalPadding = 5;

  /**
   * @brief Vertical padding above and below text in dropdown items.
   */
  static constexpr UInt16 DropdownItemVerticalPadding = 4;

  /**
   * @brief Fallback height if the font is not loaded.
   */
  static constexpr UInt16 MenuBarFallbackHeight = 20;

  /**
   * @brief Background color of the menu bar.
   */
  static constexpr UInt32 MenuBarBackgroundColor = Theme::MenuBarBackground;

  /**
   * @brief Background color of an open menu title's button.
   */
  static constexpr UInt32 MenuBarActiveBackgroundColor
    = Theme::MenuBarActiveBackground;

  /**
   * @brief Thickness of the 1px chrome border drawn along the bottom
   *        edge of the menu bar in @ref Theme::WindowBorderColor. The
   *        maximized window stretches up by this many pixels so its
   *        own top border overlaps the same row.
   */
  static constexpr UInt16 MenuBarBottomBorder = 1;

  /**
   * @brief Border color for the menu bar's bottom chrome edge.
   */
  static constexpr UInt32 MenuBarBottomBorderColor = 0xFF000000;

  /**
   * @brief Thickness of the highlight stripe drawn at the bottom of
   *        the active menu title (the one whose dropdown is open).
   *        The stripe overlays the menu bar's bottom chrome border
   *        row and eats one row upwards into the title's padding.
   */
  static constexpr UInt16 MenuBarSelectionThickness = 2;

  /**
   * @brief Text color for the active menu title (dropdown open).
   */
  static constexpr UInt32 MenuBarTextColor = Theme::MenuBarText;

  /**
   * @brief Text color for inactive menu titles and the system menu
   *        icon when their dropdown is not open.
   */
  static constexpr UInt32 MenuBarTextInactiveColor = Theme::MenuBarTextInactive;

  /**
   * @brief Background color of the dropdown panel.
   */
  static constexpr UInt32 DropdownBackgroundColor = Theme::DropdownBackground;

  /**
   * @brief Highlight color for the hovered menu item.
   */
  static constexpr UInt32 DropdownHighlightColor = Theme::HighlightColor2;

  /**
   * @brief Text color for highlighted menu items.
   */
  static constexpr UInt32 DropdownHighlightTextColor = Theme::HighlightText;

  /**
   * @brief Text color for disabled menu items.
   */
  static constexpr UInt32 DropdownDisabledTextColor = Theme::DisabledText;

  /**
   * @brief Height of a separator line in the dropdown.
   */
  static constexpr UInt16 DropdownSeparatorHeight = 12;

  /**
   * @brief Thickness of the dropdown border.
   */
  static constexpr UInt16 DropdownBorderWidth = 1;

  /**
   * @brief Horizontal inset for separator lines from the dropdown content
   *        edge.
   */
  static constexpr UInt16 DropdownSeparatorInset = 4;

  /**
   * @brief Horizontal padding on each side of a menu bar title (text or
   *        logo + chevron). Controls title spacing, hit regions, and the
   *        active highlight background.
   */
  static constexpr UInt16 MenuBarTitlePaddingX = 8;

  /**
   * @brief Width of the down-pointing dropdown chevron drawn after
   *        each non-system menu title in pixels.
   */
  static constexpr UInt16 MenuBarChevronWidth = 0;

  /**
   * @brief Height of the down-pointing dropdown chevron in pixels.
   */
  static constexpr UInt16 MenuBarChevronHeight = 0;

  /**
   * @brief Horizontal gap between the menu title text and the
   *        chevron's left edge in pixels. Adjust to taste.
   */
  static constexpr UInt16 MenuBarChevronLeftPadding = 0;

  /**
   * @brief Vertical offset applied to the chevron on top of its
   *        centered Y position, in pixels. Positive values shift
   *        the chevron down; negative values shift it up. Used to
   *        nudge the chevron off the glyph midline so it reads as
   *        a visual caret below the text rather than beside it.
   */
  static constexpr Int16 MenuBarChevronOffsetY = 1;

  /**
   * @brief Vertical padding between the dropdown border and the first/last
   *        menu item.
   */
  static constexpr UInt16 DropdownContentPadding = 6;

  /**
   * @brief Horizontal padding inside the dropdown panel.
   */
  static constexpr UInt16 DropdownPadding = 19;

  /**
   * @brief Maximum number of menus whose title hit regions are tracked.
   */
  static constexpr Size MaxTrackedMenuTitles = 16;

  static constexpr Size MenuItemTopMargin = 0;

  /**
   * @brief Hit region for a menu title in the menu bar.
   */
  struct MenuTitleHitRegion {
    /**
     * @brief Left edge of the title region in pixels.
     */
    Int16 X;

    /**
     * @brief Width of the title region in pixels.
     */
    UInt16 Width;

    /**
     * @brief Index into the current MenuBarState's menu array.
     */
    Size MenuIndex;
  };

  /**
   * @brief Manages the chromeless window that displays the system menu bar
   *        and its dropdown menus.
   *
   * Created by the context server after the application server is ready.
   * The menu bar spans the full screen width at the top of the display.
   * Dropdown panels are rendered into a separate chromeless window that
   * is shown/hidden on demand.
   */
  class MenuBar {
    public:
      /**
       * @brief Constructs the menu bar window manager.
       * @param kernel Reference to the @ref KernelClient for shared buffer
       *               operations. Must outlive this instance.
       * @param log Reference to the @ref ServerLog for logging. Must outlive
       *            this instance.
       */
      MenuBar(KernelClient& kernel, ServerLog& log)
        : _kernel(kernel), _log(log) {}

      /**
       * @brief Creates the chromeless menu bar window on the application
       *        server and loads the system font.
       * @param screenWidth The screen width in pixels.
       * @return `true` if the window was created successfully.
       */
      bool Create(UInt16 screenWidth);

      /**
       * @brief Redraws the menu bar with the given state and caches
       *        title hit regions for click detection.
       * @param state The current composed menu bar state.
       */
      void Render(const MenuBarState& state);

      /**
       * @brief Handles a mouse-down event on the menu bar.
       * @param mouseX Content-relative X coordinate.
       * @param mouseY Content-relative Y coordinate.
       * @param state The current menu bar state for dropdown rendering.
       * @return The action ID if a menu item was clicked, or 0.
       */
      ActionID HandleMouseDown(
        Int16 mouseX,
        Int16 mouseY,
        const MenuBarState& state
      );

      /**
       * @brief Handles a mouse-down event on the dropdown panel.
       * @param mouseX Content-relative X coordinate.
       * @param mouseY Content-relative Y coordinate.
       * @param state The current menu bar state.
       * @return The action ID of the clicked item, or 0.
       */
      ActionID HandleDropdownMouseDown(
        Int16 mouseX,
        Int16 mouseY,
        const MenuBarState& state
      );

      /**
       * @brief Handles a mouse-move event on the dropdown panel.
       * @param mouseX Content-relative X coordinate.
       * @param mouseY Content-relative Y coordinate.
       * @param state The current menu bar state.
       */
      void HandleDropdownMouseMove(
        Int16 mouseX,
        Int16 mouseY,
        const MenuBarState& state
      );

      /**
       * @brief Returns the dropdown window's resource ID, or 0 if none.
       */
      UInt32 GetDropdownWindowID() const { return _dropdownWindowID; }

      /**
       * @brief Returns whether the window was created successfully.
       */
      bool IsValid() const { return _valid; }

      /**
       * @brief Returns the computed menu bar height in pixels.
       */
      UInt16 GetHeight() const { return _menuBarHeight; }

      /**
       * @brief Returns the menu bar window's resource ID.
       */
      UInt32 GetWindowID() const { return _windowID; }

      /**
       * @brief Returns whether a dropdown is currently open.
       */
      bool IsDropdownOpen() const { return _dropdownOpen; }

      /**
       * @brief Closes any open dropdown.
       */
      void CloseDropdown();

    private:
      /**
       * @brief Reference to the kernel client for shared buffer operations.
       */
      KernelClient& _kernel;

      /**
       * @brief Reference to the server log.
       */
      ServerLog& _log;

      /**
       * @brief Whether the window was created successfully.
       */
      bool _valid = false;

      /**
       * @brief The AppServer-assigned window resource ID for the menu bar.
       */
      UInt32 _windowID = 0;

      /**
       * @brief The rendering surface for the menu bar.
       */
      Canvas _surface;

      /**
       * @brief The system font.
       */
      /**
       * @brief The default UI font (Helvetica) for regular menu titles
       *        and items.
       */
      Quantum::Fonts::BitmapFont _font = {};

      /**
       * @brief The system/title font (Chicago) for focus menu titles.
       */
      Quantum::Fonts::BitmapFont _titleFont = {};

      /**
       * @brief Whether the default font has been loaded.
       */
      bool _fontLoaded = false;

      /**
       * @brief Whether the title font has been loaded.
       */
      bool _titleFontLoaded = false;

      /**
       * @brief Computed menu bar height (font height + 2 * padding).
       */
      UInt16 _menuBarHeight = MenuBarFallbackHeight;

      /**
       * @brief Computed dropdown item height derived from the dropdown font's
       *        cell height plus vertical padding.
       */
      UInt16 _dropdownItemHeight = 20;

      /**
       * @brief Cached title hit regions from the last render.
       */
      MenuTitleHitRegion _titleRegions[MaxTrackedMenuTitles] = {};

      /**
       * @brief Number of valid title hit regions.
       */
      Size _titleRegionCount = 0;

      /**
       * @brief Whether a dropdown panel is currently open.
       */
      bool _dropdownOpen = false;

      /**
       * @brief The index of the currently open menu (into MenuBarState).
       */
      Size _openMenuIndex = 0;

      /**
       * @brief The index of the currently hovered item in the dropdown,
       *        or `static_cast<Size>(-1)` for none.
       */
      Size _hoveredItemIndex = static_cast<Size>(-1);

      /**
       * @brief The AppServer-assigned window resource ID for the dropdown.
       */
      UInt32 _dropdownWindowID = 0;

      /**
       * @brief The rendering surface for the dropdown panel.
       */
      Canvas _dropdownSurface;

      /**
       * @brief Whether the dropdown window has been created.
       */
      bool _dropdownCreated = false;

      /**
       * @brief Screen width for positioning.
       */
      UInt16 _screenWidth = 0;

      /**
       * @brief Loads the system font from the application server.
       * @return `true` if the font was loaded.
       */
      bool _loadFont();

      /**
       * @brief Loads the title/system font (index 1) from the AppServer.
       */
      void _loadTitleFont();

      /**
       * @brief Opens a dropdown panel for the given menu.
       * @param menuIndex Index into the current MenuBarState.
       * @param state The current menu bar state.
       */
      void _openDropdown(Size menuIndex, const MenuBarState& state);

      /**
       * @brief Renders the dropdown panel contents.
       * @param menu The menu to render.
       */
      void _renderDropdown(const Menu& menu);

      /**
       * @brief Finds which title region a point falls in.
       * @param mouseX Content-relative X coordinate.
       * @return Index into _titleRegions, or static_cast<Size>(-1).
       */
      Size _hitTestTitle(Int16 mouseX) const;
  };
}
