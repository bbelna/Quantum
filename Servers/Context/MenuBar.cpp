/**
 * @file Servers/Context/MenuBar.cpp
 * @brief Implements @ref @QCtxSrv::MenuBar.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "MenuBar.hpp"

namespace Quantum::Servers::Context {
  bool MenuBar::Create(UInt16 screenWidth) {
    _screenWidth = screenWidth;

    if (!_loadFont()) {
      _log.Write(
        LogLevel::Warning,
        "Failed to load default font"
      );
    }

    _loadTitleFont();

    // compute menu bar height from the taller font; the bottom border
    // adds an extra row that is rendered in WindowBorderColor and is
    // overlapped by the maximized window's own top border
    if (_fontLoaded || _titleFontLoaded) {
      UInt8 maxHeight = 0;

      if (_fontLoaded && _font.Height > maxHeight) {
        maxHeight = _font.Height;
      }

      if (_titleFontLoaded && _titleFont.Height > maxHeight) {
        maxHeight = _titleFont.Height;
      }

      // the content band (everything above the bottom border) must be
      // even so even-height glyphs - logo, text ink - center exactly.
      // if maxHeight would leave the band odd, grow the bottom padding
      // by 1 row so the border sits at Padding + 1 instead of Padding
      UInt16 contentBand = static_cast<UInt16>(
        maxHeight + MenuBarVerticalPadding * 2
      );

      if (contentBand % 2 != 0) contentBand++;

      _menuBarHeight = static_cast<UInt16>(contentBand + MenuBarBottomBorder);

      // dropdown items use the default font; add padding above and below
      // the glyph cell
      const auto& dropdownFont = _fontLoaded ? _font : _titleFont;

      _dropdownItemHeight = static_cast<UInt16>(
        dropdownFont.Height + DropdownItemVerticalPadding * 2
      );
    } else {
      _menuBarHeight = MenuBarFallbackHeight + MenuBarBottomBorder;
    }

    AppServer::OverlayResourceID overlayID = 0;
    AppServer::CreateOverlayResult result = {};

    bool success = AppServer::CreateOverlay(
      0,
      0,
      screenWidth,
      _menuBarHeight,
      &overlayID,
      &result
    );

    if (!success) return false;

    _windowID = overlayID;
    UIntPtr contentAddress = _kernel.AttachSharedBuffer(
      result.ContentBufferID
    );

    if (contentAddress == 0) {
      _log.Write(
        LogLevel::Error,
        "Failed to attach menu bar content buffer"
      );

      return false;
    }

    _surface = Canvas(
      reinterpret_cast<void*>(contentAddress),
      result.ContentWidth,
      result.ContentHeight,
      result.ContentStride,
      result.ContentBytesPerPixel
    );

    if (_fontLoaded) _surface.GetPainter().SetFont(_font);

    _valid = true;

    // menu bar shadow is drawn by the compositor (between background
    // and windows) so it appears below windows, not on top of them
    _surface.GetPainter().Clear(MenuBarBackgroundColor);

    return true;
  }

  void MenuBar::Render(const MenuBarState& state) {
    if (!_valid) return;

    UInt16 barWidth = _surface.GetWidth();
    UInt16 barHeight = _menuBarHeight;

    // fill the menu bar background; shadow below is static from init
    _surface.GetPainter().FillRectangle(
      Rectangle(0, 0, barWidth, barHeight),
      MenuBarBackgroundColor
    );

    // 1px chrome border along the bottom edge in WindowBorderColor;
    // a maximized window's top border draws in the same row so the
    // chrome line stays visible regardless of which one is on top
    if (MenuBarBottomBorder > 0) {
      _surface.GetPainter().FillRectangle(
        Rectangle(
          0,
          static_cast<Int16>(barHeight - MenuBarBottomBorder),
          barWidth,
          MenuBarBottomBorder
        ),
        MenuBarBottomBorderColor
      );
    }

    // a focus menu (other than the system menu itself) represents the
    // currently focused app; if any are present the system menu falls
    // back to the inactive style, otherwise it stands in for the
    // active focus and uses the active palette
    bool hasNonSystemFocusMenu = false;

    for (Size scanIndex = 0; scanIndex < state.MenuCount; ++scanIndex) {
      const Menu& scanMenu = state.Menus[scanIndex];

      if (scanMenu.IsFocusMenu && scanMenu.ID != 1) {
        hasNonSystemFocusMenu = true;

        break;
      }
    }

    // vertical centering band for titles/logos/chevrons — excludes the
    // bottom chrome border so glyphs never bleed into the border row
    UInt16 contentHeight = static_cast<UInt16>(
      _menuBarHeight - MenuBarBottomBorder
    );

    // draw menu titles and cache hit regions
    Int16 cursorX = static_cast<Int16>(MenuBarTitlePaddingX);

    _titleRegionCount = 0;

    for (Size menuIndex = 0; menuIndex < state.MenuCount; ++menuIndex) {
      const Menu& menu = state.Menus[menuIndex];

      if (menu.Title[0] == '\0') continue;

      // system menu: draw the Quantum logo instead of text
      bool isSystemMenu = menu.ID == 1;

      if (isSystemMenu) {
        UInt16 logoWidth = Bitmaps::LogoWidth;
        UInt16 logoHeight = Bitmaps::LogoHeight;
        Int16 logoY = static_cast<Int16>(
          (contentHeight - logoHeight) / 2
        );

        // the system menu also gets a chevron after its logo so it
        // matches the other menu titles; the chevron block lives
        // inside the title's hit region between the logo and the
        // trailing MenuBarTitlePaddingX
        UInt16 chevronBlockWidth = static_cast<UInt16>(
          MenuBarChevronLeftPadding + MenuBarChevronWidth
        );

        UInt16 regionWidth = static_cast<UInt16>(
          logoWidth + chevronBlockWidth + MenuBarTitlePaddingX * 2
        );

        bool selected = _dropdownOpen && _openMenuIndex == menuIndex;

        // when no app contributes a focus menu, the system menu acts
        // as the active focus and uses the active palette even when
        // its dropdown isn't open
        bool useActivePalette = selected || !hasNonSystemFocusMenu;

        // highlight background if this menu's dropdown is open; the
        // stripe spans the logo and the chevron so the whole title
        // block is underlined
        if (selected) {
          UInt16 highlightWidth = static_cast<UInt16>(
            logoWidth + chevronBlockWidth + MenuBarTitlePaddingX * 2
          );

          // full-height active background fill under the title block;
          // leaves the 1px bottom chrome border row intact so the
          // menu bar outline stays continuous across the button
          Rectangle activeFill(
            static_cast<Int16>(cursorX - MenuBarTitlePaddingX),
            0,
            highlightWidth,
            static_cast<UInt16>(_menuBarHeight - MenuBarBottomBorder)
          );

          _surface.GetPainter().FillRectangle(activeFill, MenuBarActiveBackgroundColor);

          // 2px highlight stripe at the bottom; overlays the menu
          // bar's bottom chrome border row and eats one row upwards
          // into the title's padding (mirrors the dock active button
          // top stripe)
          Rectangle highlight(
            static_cast<Int16>(cursorX - MenuBarTitlePaddingX),
            static_cast<Int16>(_menuBarHeight - MenuBarSelectionThickness),
            highlightWidth,
            MenuBarSelectionThickness
          );

          _surface.GetPainter().FillRectangle(highlight, Theme::HighlightColor);
        }

        // alpha-composite the logo into the surface
        UInt8* surfaceBytes = static_cast<UInt8*>(_surface.GetPixels());
        UInt16 surfaceStride = _surface.GetStride();
        UInt8 bpp = _surface.GetBytesPerPixel();

        for (UInt16 row = 0; row < logoHeight; ++row) {
          Int16 destY = static_cast<Int16>(logoY + row);

          if (destY < 0 || destY >= _surface.GetHeight()) continue;

          for (UInt16 col = 0; col < logoWidth; ++col) {
            Int16 destX = static_cast<Int16>(cursorX + col);

            if (destX < 0 || destX >= _surface.GetWidth()) continue;

            UInt32 sourcePixel = Bitmaps::Logo[row * logoWidth + col];
            UInt8 alpha = static_cast<UInt8>(sourcePixel >> 24);

            if (alpha == 0) continue;

            // when the system menu is in the active palette (its
            // dropdown is open OR no app contributes a focus menu),
            // render the logo in its original palette (dark mode
            // inverts so it reads light-on-dark). otherwise tint the
            // logo as a flat silhouette in MenuBarTextInactive so it
            // matches the dimmed inactive title text
            UInt8 sourceRed;
            UInt8 sourceGreen;
            UInt8 sourceBlue;

            if (useActivePalette) {
              sourceRed = static_cast<UInt8>(sourcePixel >> 16);
              sourceGreen = static_cast<UInt8>(sourcePixel >> 8);
              sourceBlue = static_cast<UInt8>(sourcePixel);

              if (Theme::DarkMode) {
                sourceRed = static_cast<UInt8>(255 - sourceRed);
                sourceGreen = static_cast<UInt8>(255 - sourceGreen);
                sourceBlue = static_cast<UInt8>(255 - sourceBlue);
              }
            } else {
              sourceRed = static_cast<UInt8>(
                MenuBarTextInactiveColor >> 16
              );
              sourceGreen = static_cast<UInt8>(
                MenuBarTextInactiveColor >> 8
              );
              sourceBlue = static_cast<UInt8>(MenuBarTextInactiveColor);
            }

            UInt8 inverseAlpha = static_cast<UInt8>(255 - alpha);

            if (bpp == 4) {
              UInt32& dest = reinterpret_cast<UInt32*>(surfaceBytes)
                [destY * surfaceStride + destX];

              UInt32 srcColor = 0xFF000000
                | (static_cast<UInt32>(sourceRed) << 16)
                | (static_cast<UInt32>(sourceGreen) << 8)
                | sourceBlue;

              dest = Quantum::Core::Color::BlendOver(srcColor, dest, alpha);
            } else {
              UInt16& dest = reinterpret_cast<UInt16*>(surfaceBytes)
                [destY * surfaceStride + destX];
              UInt8 destRed = static_cast<UInt8>((dest >> 11) << 3);
              UInt8 destGreen = static_cast<UInt8>(((dest >> 5) & 0x3F) << 2);
              UInt8 destBlue = static_cast<UInt8>((dest & 0x1F) << 3);

              UInt8 outRed = static_cast<UInt8>(
                (sourceRed * alpha + destRed * inverseAlpha) / 255
              );
              UInt8 outGreen = static_cast<UInt8>(
                (sourceGreen * alpha + destGreen * inverseAlpha) / 255
              );
              UInt8 outBlue = static_cast<UInt8>(
                (sourceBlue * alpha + destBlue * inverseAlpha) / 255
              );

              dest = static_cast<UInt16>(
                ((outRed >> 3) << 11)
                | ((outGreen >> 2) << 5)
                | (outBlue >> 3)
              );
            }
          }
        }

        // draw the down-pointing chevron after the logo, matching the
        // active/inactive palette of the logo itself
        UInt32 systemChevronColor = useActivePalette
          ? MenuBarTextColor
          : MenuBarTextInactiveColor;
        Int16 systemChevronX = static_cast<Int16>(
          cursorX + logoWidth + MenuBarChevronLeftPadding
        );
        Int16 systemChevronY = static_cast<Int16>(
          (contentHeight - MenuBarChevronHeight) / 2
        );

        for (
          UInt16 chevronRow = 0;
          chevronRow < MenuBarChevronHeight;
          ++chevronRow
        ) {
          UInt16 rowWidth = (MenuBarChevronWidth > chevronRow * 2)
            ? static_cast<UInt16>(MenuBarChevronWidth - chevronRow * 2)
            : 0;

          if (rowWidth == 0) break;

          Rectangle chevronRect(
            static_cast<Int16>(systemChevronX + chevronRow),
            static_cast<Int16>(systemChevronY + chevronRow),
            rowWidth,
            1
          );

          _surface.GetPainter().FillRectangle(chevronRect, systemChevronColor);
        }

        // cache hit region
        if (_titleRegionCount < MaxTrackedMenuTitles) {
          _titleRegions[_titleRegionCount].X = static_cast<Int16>(cursorX - MenuBarTitlePaddingX);
          _titleRegions[_titleRegionCount].Width = regionWidth;
          _titleRegions[_titleRegionCount].MenuIndex = menuIndex;
          _titleRegionCount++;
        }

        cursorX = static_cast<Int16>(cursorX + regionWidth);

        continue;
      }

      // pick font: title font for focus menus, default for others
      bool useTitleFont = true;
      const BitmapFont& activeFont = useTitleFont ? _titleFont : _font;
      bool hasFont = useTitleFont ? _titleFontLoaded : _fontLoaded;

      if (hasFont) _surface.GetPainter().SetFont(activeFont);

      Int16 textY = hasFont
        ? activeFont.CenterTextY(contentHeight)
        : static_cast<Int16>(0);

      UInt16 titleWidth = hasFont
        ? activeFont.TextWidth(menu.Title)
        : static_cast<UInt16>(CString::Length(menu.Title) * 8);

      #if QUANTUM_UI_SYNTHETIC_BOLD_ENABLED
      if (menu.IsFocusMenu) {
        titleWidth += 1;
      }
      #endif

      // each non-system menu title gets a small down-pointing chevron
      // tucked between the text and the inter-title padding. the
      // chevron lives inside the title's hit region so the existing
      // MenuBarTitlePaddingX still separates one title from the next
      UInt16 chevronBlockWidth = static_cast<UInt16>(
        MenuBarChevronLeftPadding + MenuBarChevronWidth
      );

      UInt16 regionWidth = static_cast<UInt16>(
        titleWidth + chevronBlockWidth + MenuBarTitlePaddingX * 2
      );

      // draw the title text first; the open-menu indicator is a 2px
      // stripe at the bottom (mimicking the dock active button). the
      // open menu and any focus menu (the focused app's identity)
      // use MenuBarTextColor, all other titles render dimmed in
      // MenuBarTextInactiveColor
      bool selected = _dropdownOpen && _openMenuIndex == menuIndex;
      UInt32 titleColor = (selected || menu.IsFocusMenu)
        ? MenuBarTextColor
        : MenuBarTextInactiveColor;

      // full-height active background fill under the title block
      // when this menu's dropdown is open; drawn before the text so
      // glyph AA composites against the correct background. leaves
      // the 1px bottom chrome border row intact
      UInt32 textBackground = MenuBarBackgroundColor;

      if (selected) {
        UInt16 activeFillWidth = static_cast<UInt16>(
          titleWidth + chevronBlockWidth + MenuBarTitlePaddingX * 2
        );
        Rectangle activeFill(
          static_cast<Int16>(cursorX - MenuBarTitlePaddingX),
          0,
          activeFillWidth,
          static_cast<UInt16>(_menuBarHeight - MenuBarBottomBorder)
        );

        _surface.GetPainter().FillRectangle(activeFill, MenuBarActiveBackgroundColor);
        textBackground = MenuBarActiveBackgroundColor;
      }

      if (menu.IsFocusMenu) {
        _surface.GetPainter().SetFont(_titleFont);
      } else {
        _surface.GetPainter().SetFont(_font);
      }

      _surface.GetPainter().DrawText(
        cursorX,
        textY,
        menu.Title,
        titleColor,
        textBackground,
        menu.IsFocusMenu
      );

      // draw the down-pointing dropdown chevron after the title text;
      // each row is 2px narrower than the one above, centered, so the
      // shape forms a triangle pointing down. the chevron is centered
      // vertically within the same content band as the text
      Int16 chevronX = static_cast<Int16>(
        cursorX + titleWidth + MenuBarChevronLeftPadding
      );
      Int16 chevronY = static_cast<Int16>(
        (contentHeight - MenuBarChevronHeight) / 2
      );

      for (
        UInt16 chevronRow = 0;
        chevronRow < MenuBarChevronHeight;
        ++chevronRow
      ) {
        UInt16 rowWidth = (MenuBarChevronWidth > chevronRow * 2)
          ? static_cast<UInt16>(MenuBarChevronWidth - chevronRow * 2)
          : 0;

        if (rowWidth == 0) break;

        Rectangle chevronRect(
          static_cast<Int16>(chevronX + chevronRow),
          static_cast<Int16>(chevronY + chevronRow),
          rowWidth,
          1
        );

        _surface.GetPainter().FillRectangle(chevronRect, titleColor);
      }

      // 2px highlight stripe at the bottom for the open menu; overlays
      // the menu bar's bottom chrome border row and eats one row
      // upwards into the title's padding. the stripe spans the text
      // and the chevron so the whole title block is underlined
      if (selected) {
        UInt16 highlightWidth = static_cast<UInt16>(
          titleWidth + chevronBlockWidth + MenuBarTitlePaddingX * 2
        );

        Rectangle highlight(
          static_cast<Int16>(cursorX - MenuBarTitlePaddingX),
          static_cast<Int16>(_menuBarHeight - MenuBarSelectionThickness),
          highlightWidth,
          MenuBarSelectionThickness
        );

        _surface.GetPainter().FillRectangle(highlight, Theme::HighlightColor);
      }

      // cache hit region
      if (_titleRegionCount < MaxTrackedMenuTitles) {
        _titleRegions[_titleRegionCount].X = static_cast<Int16>(cursorX - MenuBarTitlePaddingX);
        _titleRegions[_titleRegionCount].Width = regionWidth;
        _titleRegions[_titleRegionCount].MenuIndex = menuIndex;
        _titleRegionCount++;
      }

      cursorX = static_cast<Int16>(cursorX + regionWidth);
    }

    AppServer::InvalidateOverlay(_windowID);
  }

  ActionID MenuBar::HandleMouseDown(
    Int16 mouseX,
    Int16 mouseY,
    const MenuBarState& state
  ) {
    Size hitIndex = _hitTestTitle(mouseX);

    if (hitIndex != static_cast<Size>(-1)) {
      Size menuIndex = _titleRegions[hitIndex].MenuIndex;

      if (_dropdownOpen && _openMenuIndex == menuIndex) {
        // clicking the same title closes the dropdown
        CloseDropdown();
        Render(state);
      } else {
        // open the dropdown for this menu
        _openDropdown(menuIndex, state);
        Render(state);
      }
    } else if (_dropdownOpen) {
      // clicked outside all titles, close dropdown
      CloseDropdown();
      Render(state);
    }

    return 0;
  }

  void MenuBar::CloseDropdown() {
    if (!_dropdownOpen) return;

    _dropdownOpen = false;
    _hoveredItemIndex = static_cast<Size>(-1);

    if (_dropdownCreated) {
      // detach our local mapping of the dropdown's shared content buffer
      // before asking the AppServer to close it; this ensures the kernel's
      // reference count reaches zero and the buffer is freed
      if (_dropdownSurface.IsValid()) {
        _kernel.DetachSharedBuffer(
          reinterpret_cast<UIntPtr>(_dropdownSurface.GetPixels())
        );

        _dropdownSurface = Canvas();
      }

      AppServer::CloseOverlay(_dropdownWindowID);

      _dropdownCreated = false;
      _dropdownWindowID = 0;
    }
  }

  void MenuBar::_openDropdown(
    Size menuIndex,
    const MenuBarState& state
  ) {
    if (menuIndex >= state.MenuCount) return;

    const Menu& menu = state.Menus[menuIndex];

    if (menu.ItemCount == 0) return;

    // close any existing dropdown
    CloseDropdown();

    // calculate dropdown dimensions
    UInt16 dropdownWidth = 180;
    UInt16 dropdownHeight = 0;
    Size visibleCount = 0;

    for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
      const MenuItem& item = menu.Items[itemIndex];

      if (item.State == ActionState::Hidden) continue;

      if (item.SeparatorBefore) {
        dropdownHeight += DropdownSeparatorHeight;
      } else if (visibleCount > 0) {
        // inter-item padding before every non-first item that isn't
        // already separated by a separator line
        dropdownHeight += MenuItemTopMargin;
      }

      dropdownHeight += _dropdownItemHeight;

      // measure item width to potentially widen the dropdown
      if (_titleFontLoaded || _fontLoaded) {
        const auto& measureFont = _titleFont;
        UInt16 itemWidth = measureFont.TextWidth(item.Label) + DropdownPadding * 2;

        if (itemWidth > dropdownWidth) {
          dropdownWidth = itemWidth;
        }
      }

      visibleCount++;
    }

    // add space for 1px border on left/right/bottom (no top border so
    // the menu bar's selection stripe sits flush against the dropdown)
    // plus content padding top/bottom
    dropdownWidth = static_cast<UInt16>(
      dropdownWidth + DropdownBorderWidth * 2
    );
    dropdownHeight = static_cast<UInt16>(
      dropdownHeight + DropdownBorderWidth + DropdownContentPadding * 2
    );

    // position the dropdown below the title
    Int16 dropdownX = 0;

    for (Size index = 0; index < _titleRegionCount; ++index) {
      if (_titleRegions[index].MenuIndex == menuIndex) {
        dropdownX = _titleRegions[index].X;

        break;
      }
    }

    // dropdownX is the left edge of the title's hit/highlight region;
    // nudge it one pixel left so the dropdown visually aligns with the
    // title's glyphs rather than the highlight padding. no clamp -
    // CreateOverlay accepts negative X and the compositor clips, so a
    // dropdown whose title sits at X=0 simply loses its leftmost column
    dropdownX = static_cast<Int16>(dropdownX - 1);

    // sit flush below the menu bar - no overlap; the dropdown has no
    // top border so the menu bar's selection stripe stays visible
    Int16 dropdownY = static_cast<Int16>(_menuBarHeight);

    // expand overlay to accommodate the soft shadow around the content;
    // shadow appears on left, right, and bottom (top connects to menu bar)
    // margins account for the global shadow offset
    UInt16 shadowMarginLeft = 0;
    UInt16 shadowMarginRight = 0;
    UInt16 shadowMarginBottom = 0;

    if (Theme::EnableShadows) {
      shadowMarginLeft = Theme::ShadowSize;
      shadowMarginRight = Theme::ShadowSize;
      shadowMarginBottom = Theme::ShadowSize;
    }

    UInt16 surfaceWidth = static_cast<UInt16>(
      dropdownWidth + shadowMarginLeft + shadowMarginRight
    );
    UInt16 surfaceHeight = static_cast<UInt16>(
      dropdownHeight + shadowMarginBottom
    );
    Int16 overlayX = static_cast<Int16>(
      dropdownX - shadowMarginLeft
    );

    // create the dropdown overlay
    AppServer::OverlayResourceID dropdownID = 0;
    AppServer::CreateOverlayResult dropdownResult = {};

    bool success = AppServer::CreateOverlay(
      overlayX,
      dropdownY,
      surfaceWidth,
      surfaceHeight,
      &dropdownID,
      &dropdownResult
    );

    if (!success) {
      _log.Write(
        LogLevel::Warning,
        "Failed to create dropdown overlay"
      );

      return;
    }

    _dropdownWindowID = dropdownID;
    _dropdownCreated = true;

    UIntPtr dropdownAddress = _kernel.AttachSharedBuffer(
      dropdownResult.ContentBufferID
    );

    if (dropdownAddress == 0) return;

    _dropdownSurface = Canvas(
      reinterpret_cast<void*>(dropdownAddress),
      dropdownResult.ContentWidth,
      dropdownResult.ContentHeight,
      dropdownResult.ContentStride,
      dropdownResult.ContentBytesPerPixel
    );

    // if (_titleFontLoaded) {
    //   _dropdownSurface.GetPainter().SetFont(_titleFont);
    // } else if (_fontLoaded) {
    _dropdownSurface.GetPainter().SetFont(_titleFont);
    // }

    _dropdownOpen = true;
    _openMenuIndex = menuIndex;

    _renderDropdown(menu);
  }

  void MenuBar::_renderDropdown(const Menu& menu) {
    UInt16 surfaceWidth = _dropdownSurface.GetWidth();
    UInt16 surfaceHeight = _dropdownSurface.GetHeight();

    // the content rect is inset from the surface by the shadow margins
    UInt16 shadowMarginLeft = 0;
    UInt16 shadowMarginRight = 0;
    UInt16 shadowMarginBottom = 0;

    if (Theme::EnableShadows) {
      shadowMarginLeft = Theme::ShadowSize;
      shadowMarginRight = Theme::ShadowSize;
      shadowMarginBottom = Theme::ShadowSize;
    }

    UInt16 contentWidth = static_cast<UInt16>(
      surfaceWidth - shadowMarginLeft - shadowMarginRight
    );
    UInt16 contentHeight = static_cast<UInt16>(
      surfaceHeight - shadowMarginBottom
    );
    Int16 contentLeft = static_cast<Int16>(shadowMarginLeft);

    // when the dropdown corner radius is zero, fall through every
    // rounded path so the dropdown is rendered as a flat rectangle and
    // any future rounded-only logic stays gated behind this flag
    constexpr bool useRoundedCorners = Theme::DropdownBorderRadius > 0;
    constexpr RoundedCorners dropdownCorners = useRoundedCorners
      ? (RoundedCorners::BottomLeft | RoundedCorners::BottomRight)
      : RoundedCorners::None;

    // clear entire surface to transparent so corner/shadow pixels are
    // invisible to the compositor
    _dropdownSurface.GetPainter().Clear(0x00000000);

    // render shadow using the centralized shadow utility
    Rectangle contentInSurface(
      contentLeft,
      0,
      contentWidth,
      contentHeight
    );

    if (Theme::EnableShadows) {
      _dropdownSurface.GetPainter().RenderShadow(
        contentInSurface,
        Theme::ShadowSize,
        Theme::ShadowAlpha,
        useRoundedCorners ? Theme::DropdownBorderRadius : 0,
        static_cast<Int16>(Theme::ShadowOffsetX),
        static_cast<Int16>(Theme::ShadowOffsetY),
        dropdownCorners
      );
    }

    // fill content area: first a border-colored rect, then an interior
    // rect inset by the border width on left/right/bottom only. the
    // top edge has no border so the menu bar's selection stripe
    // remains visible flush against the dropdown's interior. when
    // corners are enabled the edges use the rounded path; otherwise
    // both fall through to a flat FillRectangle.
    Rectangle contentRect(contentLeft, 0, contentWidth, contentHeight);

    UInt32 borderColor = Theme::Shadow;

    Rectangle interiorRect(
      static_cast<Int16>(contentLeft + DropdownBorderWidth),
      0,
      static_cast<UInt16>(contentWidth - DropdownBorderWidth * 2),
      static_cast<UInt16>(contentHeight - DropdownBorderWidth)
    );

    if (useRoundedCorners) {
      UInt8 interiorRadius = Theme::DropdownBorderRadius > DropdownBorderWidth
        ? static_cast<UInt8>(
            Theme::DropdownBorderRadius - DropdownBorderWidth
          )
        : 0;

      _dropdownSurface.GetPainter().FillRoundedRectangle(
        contentRect,
        borderColor,
        Theme::DropdownBorderRadius,
        dropdownCorners
      );

      _dropdownSurface.GetPainter().FillRoundedRectangle(
        interiorRect,
        DropdownBackgroundColor,
        interiorRadius,
        dropdownCorners
      );
    } else {
      _dropdownSurface.GetPainter().FillRectangle(contentRect, borderColor);
      _dropdownSurface.GetPainter().FillRectangle(interiorRect, DropdownBackgroundColor);
    }

    // draw menu items within the content area
    UInt16 itemAreaWidth = static_cast<UInt16>(
      contentWidth - DropdownBorderWidth * 2
    );

    // first item starts at the top content padding; there is no top
    // border to skip past
    Int16 itemY = static_cast<Int16>(DropdownContentPadding);

    const auto& dropdownFont = _fontLoaded ? _font : _titleFont;
    bool hasDropdownFont = _fontLoaded || _titleFontLoaded;

    Int16 textYOffset = hasDropdownFont
      ? dropdownFont.CenterTextY(_dropdownItemHeight)
      : static_cast<Int16>(0);

    Size visibleIndex = 0;

    for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
      const MenuItem& item = menu.Items[itemIndex];

      if (item.State == ActionState::Hidden) continue;

      // draw separator
      if (item.SeparatorBefore) {
        Int16 separatorY = static_cast<Int16>(
          itemY + DropdownSeparatorHeight / 2
        );

        Rectangle separatorLine(
          static_cast<Int16>(
            contentLeft + DropdownBorderWidth + DropdownSeparatorInset
          ),
          separatorY,
          static_cast<UInt16>(
            itemAreaWidth - DropdownSeparatorInset * 2
          ),
          1
        );

        _dropdownSurface.GetPainter().FillRectangle(separatorLine, Theme::SeparatorColor);

        itemY += static_cast<Int16>(DropdownSeparatorHeight);
      } else if (visibleIndex > 0) {
        // inter-item padding before every non-first item that isn't
        // already separated by a separator line
        itemY += static_cast<Int16>(MenuItemTopMargin);
      }

      // determine colors based on hover state
      bool hovered = (visibleIndex == _hoveredItemIndex);
      bool disabled = (item.State == ActionState::Disabled);

      UInt32 bgColor = DropdownBackgroundColor;
      UInt32 textColor = MenuBarTextColor;

      if (disabled) {
        textColor = DropdownDisabledTextColor;
      } else if (hovered) {
        bgColor = DropdownHighlightColor;
        textColor = DropdownHighlightTextColor;

        Rectangle highlightRect(
          static_cast<Int16>(contentLeft + DropdownBorderWidth),
          itemY,
          itemAreaWidth,
          _dropdownItemHeight
        );

        _dropdownSurface.GetPainter().FillRectangle(highlightRect, bgColor);
      }

      _dropdownSurface.GetPainter().DrawText(
        static_cast<Int16>(
          contentLeft + DropdownPadding + DropdownBorderWidth
        ),
        static_cast<Int16>(itemY + textYOffset),
        item.Label,
        textColor,
        bgColor
      );

      itemY = static_cast<Int16>(itemY + _dropdownItemHeight);
      visibleIndex++;
    }

    AppServer::InvalidateOverlay(_dropdownWindowID);
  }

  ActionID MenuBar::HandleDropdownMouseDown(
    Int16 mouseX,
    Int16 mouseY,
    const MenuBarState& state
  ) {
    if (!_dropdownOpen || _openMenuIndex >= state.MenuCount) {
      return 0;
    }

    // clicks in the shadow margin close the dropdown
    UInt16 marginLeft = 0;
    UInt16 marginBottom = 0;

    if (Theme::EnableShadows) {
      marginLeft = Theme::ShadowSize;
      marginBottom = Theme::ShadowSize;
    }

    Int16 contentMouseX = static_cast<Int16>(mouseX - marginLeft);
    UInt16 contentWidth = static_cast<UInt16>(
      _dropdownSurface.GetWidth() - marginLeft
      - ((Theme::EnableShadows) ? Theme::ShadowSize : 0)
    );
    UInt16 contentHeight = static_cast<UInt16>(
      _dropdownSurface.GetHeight() - marginBottom
    );

    if (
      contentMouseX < 0
      || contentMouseX >= static_cast<Int16>(contentWidth)
      || mouseY < 0
      || mouseY >= static_cast<Int16>(contentHeight)
    ) {
      CloseDropdown();
      Render(state);

      return 0;
    }

    const Menu& menu = state.Menus[_openMenuIndex];

    // hit test: find which visible item was clicked
    // first item starts at the top content padding; there is no top
    // border to skip past
    Int16 itemY = static_cast<Int16>(DropdownContentPadding);
    Size visibleIndex = 0;

    for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
      const MenuItem& item = menu.Items[itemIndex];

      if (item.State == ActionState::Hidden) continue;

      if (item.SeparatorBefore) {
        itemY += static_cast<Int16>(DropdownSeparatorHeight);
      } else if (visibleIndex > 0) {
        itemY += static_cast<Int16>(MenuItemTopMargin);
      }

      Int16 itemBottom = static_cast<Int16>(itemY + _dropdownItemHeight);

      if (mouseY >= itemY && mouseY < itemBottom) {
        if (item.State != ActionState::Disabled) {
          ActionID action = item.Action;

          CloseDropdown();
          Render(state);

          return action;
        }

        return 0;
      }

      itemY = itemBottom;
      visibleIndex++;
    }

    return 0;
  }

  void MenuBar::HandleDropdownMouseMove(
    Int16 mouseX,
    Int16 mouseY,
    const MenuBarState& state
  ) {
    if (!_dropdownOpen || _openMenuIndex >= state.MenuCount) return;

    // if mouse is in the shadow margin, clear hover
    UInt16 marginLeft = 0;
    UInt16 marginBottom = 0;

    if (Theme::EnableShadows) {
      marginLeft = Theme::ShadowSize;
      marginBottom = Theme::ShadowSize;
    }

    Int16 contentMouseX = static_cast<Int16>(mouseX - marginLeft);
    UInt16 contentWidth = static_cast<UInt16>(
      _dropdownSurface.GetWidth() - marginLeft
      - ((Theme::EnableShadows) ? Theme::ShadowSize : 0)
    );
    UInt16 contentHeight = static_cast<UInt16>(
      _dropdownSurface.GetHeight() - marginBottom
    );

    if (
      contentMouseX < 0
      || contentMouseX >= static_cast<Int16>(contentWidth)
      || mouseY < 0
      || mouseY >= static_cast<Int16>(contentHeight)
    ) {
      if (_hoveredItemIndex != static_cast<Size>(-1)) {
        _hoveredItemIndex = static_cast<Size>(-1);

        const Menu& menu = state.Menus[_openMenuIndex];

        _renderDropdown(menu);
      }

      return;
    }

    const Menu& menu = state.Menus[_openMenuIndex];

    // hit test to find hovered item
    // first item starts at the top content padding; there is no top
    // border to skip past
    Int16 itemY = static_cast<Int16>(DropdownContentPadding);
    Size visibleIndex = 0;
    Size newHovered = static_cast<Size>(-1);

    for (Size itemIndex = 0; itemIndex < menu.ItemCount; ++itemIndex) {
      const MenuItem& item = menu.Items[itemIndex];

      if (item.State == ActionState::Hidden) continue;

      if (item.SeparatorBefore) {
        itemY += static_cast<Int16>(DropdownSeparatorHeight);
      } else if (visibleIndex > 0) {
        itemY += static_cast<Int16>(MenuItemTopMargin);
      }

      Int16 itemBottom = static_cast<Int16>(itemY + _dropdownItemHeight);

      if (
        mouseY >= itemY && mouseY < itemBottom &&
        item.State != ActionState::Disabled
      ) {
        newHovered = visibleIndex;
      }

      itemY = itemBottom;
      visibleIndex++;
    }

    if (newHovered != _hoveredItemIndex) {
      _hoveredItemIndex = newHovered;

      _renderDropdown(menu);
    }
  }

  Size MenuBar::_hitTestTitle(Int16 mouseX) const {
    for (Size index = 0; index < _titleRegionCount; ++index) {
      Int16 left = _titleRegions[index].X;
      Int16 right = static_cast<Int16>(
        left + _titleRegions[index].Width
      );

      if (mouseX >= left && mouseX < right) return index;
    }

    return static_cast<Size>(-1);
  }

  bool MenuBar::_loadFont() {
    AppServer::GetSystemFontsResult fonts = {};

    if (!AppServer::GetSystemFonts(&fonts)) return false;
    if (fonts.FontCount == 0 || fonts.Fonts[0].BufferID == 0) return false;

    UIntPtr fontAddress = _kernel.AttachSharedBuffer(
      fonts.Fonts[0].BufferID
    );

    if (fontAddress == 0) return false;

    auto* data = reinterpret_cast<const UInt8*>(fontAddress);
    Size size = static_cast<Size>(fonts.Fonts[0].DataSize);

    _font = Quantum::Fonts::QBFParser::Parse(data, size);

    if (!_font.GlyphData) {
      _font = Quantum::Fonts::PSFParser::Parse(data, size);
    }

    if (_font.GlyphData) {
      _fontLoaded = true;

      return true;
    }

    _kernel.DetachSharedBuffer(fontAddress);

    return false;
  }

  void MenuBar::_loadTitleFont() {
    AppServer::GetSystemFontsResult fonts = {};

    if (!AppServer::GetSystemFonts(&fonts)) return;
    if (fonts.FontCount < 2 || fonts.Fonts[1].BufferID == 0) return;

    UIntPtr fontAddress = _kernel.AttachSharedBuffer(
      fonts.Fonts[1].BufferID
    );

    if (fontAddress == 0) return;

    auto* data = reinterpret_cast<const UInt8*>(fontAddress);
    Size size = static_cast<Size>(fonts.Fonts[1].DataSize);

    _titleFont = Quantum::Fonts::QBFParser::Parse(data, size);

    if (!_titleFont.GlyphData) {
      _titleFont = Quantum::Fonts::PSFParser::Parse(data, size);
    }

    if (_titleFont.GlyphData) {
      _titleFontLoaded = true;

      return;
    }

    _kernel.DetachSharedBuffer(fontAddress);
  }
}
