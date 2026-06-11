/**
 * @file Servers/App/Dock/Dock.cpp
 * @brief Implements @ref @QAppSrv::Dock::Dock.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Dock.hpp"
#include "Windows/WindowManager.hpp"
#include <Quantum/Core/Color.hpp>
#include <Quantum/Core/Math.hpp>

namespace Quantum::Servers::App::Dock {
  using Math::CornerInset;
  void Dock::Init(
    WindowManager* windowManager,
    UInt16 screenWidth,
    UInt16 screenHeight,
    const Fonts::BitmapFont* activeFont,
    const Fonts::BitmapFont* inactiveFont
  ) {
    _windowManager = windowManager;
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    _activeFont = activeFont;
    _inactiveFont = inactiveFont;

    // size the buttons from the taller of the two fonts
    UInt8 maxFontHeight = 0;

    if (activeFont && activeFont->Height > maxFontHeight) {
      maxFontHeight = activeFont->Height;
    }

    if (inactiveFont && inactiveFont->Height > maxFontHeight) {
      maxFontHeight = inactiveFont->Height;
    }

    _buttonHeight = maxFontHeight > 0
      ? static_cast<UInt16>(maxFontHeight + ButtonVerticalPadding * 2)
      : ButtonFallbackHeight;

    UInt16 bezelTotal = BottomBezel
      ? static_cast<UInt16>(BezelThickness * 2)
      : BezelThickness;

    // the top border adds an extra row that is rendered in
    // WindowBorderColor and is overlapped by a maximized window's
    // own bottom border
    _height = static_cast<UInt16>(
      bezelTotal + Padding * 2 + _buttonHeight + TopBorder
    );

    _surfaceWidth = screenWidth;
    _surfaceHeight = _height;

    Size pixelCount = static_cast<Size>(_surfaceWidth) * _surfaceHeight;

    _surface = new UInt32[pixelCount];
    _dirty = true;
  }

  Rectangle Dock::GetFrame() const {
    return Rectangle(
      0,
      static_cast<Int16>(_screenHeight - _height),
      _screenWidth,
      _height
    );
  }

  bool Dock::Contains(Int16 screenX, Int16 screenY) const {
    return GetFrame().Contains(Point(screenX, screenY));
  }

  UInt32 Dock::HitTest(Int16 screenX, Int16 screenY) const {
    Int16 dockTop = static_cast<Int16>(_screenHeight - _height);
    Int16 localX = screenX;
    Int16 localY = static_cast<Int16>(screenY - dockTop);

    if (localY < 0 || localY >= _height) return 0;

    for (Size i = 0; i < _buttonCount; ++i) {
      const DockButton& button = _buttons[i];
      Int16 buttonLeft = button.X;
      Int16 buttonRight = static_cast<Int16>(button.X + button.Width);
      Int16 buttonTop = Padding > 0
        ? static_cast<Int16>(BezelThickness + Padding + TopBorder)
        : ButtonsOverlapBezel
          ? static_cast<Int16>(0)
          : static_cast<Int16>(_height - _buttonHeight);
      UInt16 actual_buttonHeight = ButtonsOverlapBezel
        ? _height
        : _buttonHeight;
      Int16 buttonBottom = static_cast<Int16>(buttonTop + actual_buttonHeight);

      if (
        localX >= buttonLeft &&
        localX < buttonRight &&
        localY >= buttonTop &&
        localY < buttonBottom
      ) return button.WindowResourceID;
    }

    return 0;
  }

  void Dock::Rebuild() {
    _buttonCount = 0;

    if (!_windowManager) return;

    // collect non-chromeless windows
    Size resourceCount = _windowManager->GetResourceCount();
    PathNode<Window*>* activeNode = _windowManager->GetActiveNode();

    for (Size i = 0; i < resourceCount; ++i) {
      if (_buttonCount >= MaxButtons) break;

      const WindowResource& resource = _windowManager->GetResource(i);

      if (!resource.Ptr) continue;
      if (resource.Ptr->IsChromeless()) continue;

      // skip modal dialogs, they're represented by their parent window
      bool isModalDialog = false;

      for (Size j = 0; j < resourceCount; ++j) {
        if (_windowManager->GetResource(j).ModalDialogID == resource.ID) {
          isModalDialog = true;

          break;
        }
      }

      if (isModalDialog) continue;

      DockButton& button = _buttons[_buttonCount];

      button.WindowResourceID = resource.ID;

      // focused if this window is active, OR if a dialog of this window
      // is active (the dialog is hidden from the dock)
      bool isFocused = false;

      if (activeNode && !resource.Ptr->IsMinimized()) {
        if (activeNode->GetValue() == resource.Ptr) {
          isFocused = true;
        } else if (resource.ModalDialogID != 0) {
          // this window has a dialog, check if the dialog is active
          for (Size j = 0; j < resourceCount; ++j) {
            if (
              _windowManager->GetResource(j).ID == resource.ModalDialogID &&
              _windowManager->GetResource(j).Ptr == activeNode->GetValue()
            ) {
              isFocused = true;

              break;
            }
          }
        }
      }

      button.Focused = isFocused;
      button.Minimized = resource.Ptr->IsMinimized();

      // copy title
      const char* title = resource.Ptr->GetTitle();
      Size titleIndex = 0;

      if (title) {
        for (; title[titleIndex] && titleIndex < 31; ++titleIndex) {
          button.Title[titleIndex] = title[titleIndex];
        }
      }

      button.Title[titleIndex] = '\0';

      _buttonCount++;
    }

    // compute button widths and positions
    UInt16 availableWidth = static_cast<UInt16>(
      _screenWidth - Padding * 2
    );

    UInt16 gapWidth = ButtonSeparator
      ? static_cast<UInt16>(ButtonSpacing * 2 + ButtonSeparatorWidth)
      : ButtonSpacing;

    const Fonts::BitmapFont* measureFont =
      _activeFont ? _activeFont : _inactiveFont;

    // when ButtonDefaultWidth == 0, size each button to fit its text;
    // otherwise use the fixed default width for all buttons
    if (ButtonDefaultWidth == 0 && measureFont) {
      // first pass: measure natural width for each button
      UInt16 totalNeeded = 0;

      for (Size i = 0; i < _buttonCount; ++i) {
        UInt16 textWidth = measureFont->TextWidth(_buttons[i].Title);

        // subtract the trailing advance of the last character so the
        // button fits tightly around the visible ink
        Size titleLength = CString::Length(_buttons[i].Title);

        if (titleLength > 0 && measureFont->IsProportional()) {
          UInt8 lastChar = static_cast<UInt8>(
            _buttons[i].Title[titleLength - 1]
          );
          UInt8 lastAdvance = measureFont->GetAdvance(lastChar);
          UInt8 glyphWidth = measureFont->Width;

          if (lastAdvance > glyphWidth) {
            textWidth = static_cast<UInt16>(
              textWidth - (lastAdvance - glyphWidth)
            );
          }
        }

        UInt16 natural = static_cast<UInt16>(textWidth + TextPadding * 2);

        if (natural < ButtonMinWidth) natural = ButtonMinWidth;

        _buttons[i].Width = natural;
        totalNeeded = static_cast<UInt16>(totalNeeded + natural);
      }

      if (_buttonCount > 1) {
        totalNeeded = static_cast<UInt16>(
          totalNeeded + (_buttonCount - 1) * gapWidth
        );
      }

      // if total exceeds available space, shrink proportionally
      if (totalNeeded > availableWidth && totalNeeded > 0) {
        UInt16 totalSpacing = _buttonCount > 1
          ? static_cast<UInt16>((_buttonCount - 1) * gapWidth)
          : 0;
        UInt16 spaceForButtons = static_cast<UInt16>(
          availableWidth > totalSpacing ? availableWidth - totalSpacing : 0
        );
        UInt16 totalButtonWidth = static_cast<UInt16>(
          totalNeeded - totalSpacing
        );

        for (Size i = 0; i < _buttonCount; ++i) {
          _buttons[i].Width = static_cast<UInt16>(
            static_cast<UInt32>(_buttons[i].Width)
            * spaceForButtons / totalButtonWidth
          );

          if (_buttons[i].Width < ButtonMinWidth) {
            _buttons[i].Width = ButtonMinWidth;
          }
        }

        _isFull = true;
      } else {
        _isFull = false;
      }
    } else {
      // fixed-width mode
      UInt16 buttonWidth = ButtonDefaultWidth;

      if (_buttonCount > 0) {
        UInt16 totalSpacing = static_cast<UInt16>(
          (_buttonCount - 1) * gapWidth
        );
        UInt16 neededWidth = static_cast<UInt16>(
          _buttonCount * ButtonDefaultWidth + totalSpacing
        );

        if (neededWidth > availableWidth) {
          UInt16 spaceForButtons = static_cast<UInt16>(
            availableWidth > totalSpacing
              ? availableWidth - totalSpacing : 0
          );

          buttonWidth = static_cast<UInt16>(
            spaceForButtons / _buttonCount
          );

          if (buttonWidth < ButtonMinWidth) buttonWidth = ButtonMinWidth;

          _isFull = true;
        } else {
          _isFull = false;
        }
      } else {
        _isFull = false;
      }

      for (Size i = 0; i < _buttonCount; ++i) {
        _buttons[i].Width = buttonWidth;
      }
    }

    // second pass: assign positions and truncate titles to fit
    Int16 cursorX = static_cast<Int16>(Padding);

    for (Size i = 0; i < _buttonCount; ++i) {
      _buttons[i].X = cursorX;

      UInt16 buttonWidth = _buttons[i].Width;

      if (measureFont) {
        UInt16 maxTextWidth = static_cast<UInt16>(
          buttonWidth > TextPadding * 2
            ? buttonWidth - TextPadding * 2
            : 0
        );

        UInt16 titleLength = static_cast<UInt16>(
          CString::Length(_buttons[i].Title)
        );

        if (measureFont->TextWidth(_buttons[i].Title) > maxTextWidth) {
          UInt16 ellipsisWidth = measureFont->TextWidth("...");
          UInt16 budget = static_cast<UInt16>(
            maxTextWidth > ellipsisWidth
              ? maxTextWidth - ellipsisWidth
              : 0
          );
          UInt16 accumulated = 0;
          UInt16 cutoff = 0;

          for (UInt16 charIndex = 0; charIndex < titleLength; ++charIndex) {
            UInt8 character = static_cast<UInt8>(
              _buttons[i].Title[charIndex]
            );
            UInt16 advance = measureFont->GetAdvance(character);

            if (accumulated + advance > budget) break;

            accumulated = static_cast<UInt16>(accumulated + advance);
            cutoff = static_cast<UInt16>(charIndex + 1);
          }

          _buttons[i].Title[cutoff] = '.';
          _buttons[i].Title[cutoff + 1] = '.';
          _buttons[i].Title[cutoff + 2] = '.';
          _buttons[i].Title[cutoff + 3] = '\0';
        }
      }

      cursorX = static_cast<Int16>(
        cursorX + buttonWidth + gapWidth
      );
    }

    _renderSurface();

    _dirty = false;
  }

  void Dock::_renderSurface() {
    if (!_surface) return;

    Size pixelCount = static_cast<Size>(_surfaceWidth) * _surfaceHeight;

    // fill background (skip the top bezel row, written separately as shadow)
    UInt32 backgroundStart = static_cast<UInt32>(BezelThickness) * _surfaceWidth;

    for (Size i = backgroundStart; i < pixelCount; ++i) {
      _surface[i] = Theme::DockBackground;
    }

    // top shadow (raw ARGB for compositor alpha blending)
    for (UInt16 layer = 0; layer < BezelThickness; ++layer) {
      UInt32 shadowColor = Theme::EnableTranslucency
        ? Theme::Shadow
        : (Theme::Shadow | 0xFF000000);

      for (UInt16 x = 0; x < _surfaceWidth; ++x) {
        _surface[layer * _surfaceWidth + x] = shadowColor;
      }
    }

    // 1px chrome border along the top edge in WindowBorderColor;
    // a maximized window's bottom border draws in the same row so the
    // chrome line stays visible regardless of which one is on top
    if (TopBorder > 0) {
      UInt32 topBorderColor = Theme::WindowBorderColor;

      for (UInt16 layer = 0; layer < TopBorder; ++layer) {
        UInt32 row = static_cast<UInt32>(BezelThickness + layer)
          * _surfaceWidth;

        for (UInt16 x = 0; x < _surfaceWidth; ++x) {
          _surface[row + x] = topBorderColor;
        }
      }
    }

    // draw buttons
    for (Size i = 0; i < _buttonCount; ++i) {
      const DockButton& button = _buttons[i];
      Int16 buttonTop = ButtonsOverlapBezel
        ? static_cast<Int16>(0)
        : static_cast<Int16>(BezelThickness + Padding + TopBorder);
      UInt16 actual_buttonHeight = ButtonsOverlapBezel
        ? _surfaceHeight
        : _buttonHeight;
      Int16 buttonLeft = button.X;
      UInt16 buttonWidth = button.Width;

      {
        UInt32 buttonColor = button.Focused
          ? Theme::DockButtonActiveBackground
          : Theme::DockButtonDefaultBackground;
        UInt8 btnRadius = Theme::DockButtonRoundedCorners
          ? Theme::DockButtonBorderRadius
          : 0;

        for (UInt16 row = 0; row < actual_buttonHeight; ++row) {
          // compute left/right insets for rounded corners
          Int16 leftInset = 0;
          Int16 rightInset = 0;

          if (row < btnRadius) {
            leftInset = CornerInset(btnRadius, row);
            rightInset = leftInset;
          }

          if (row >= actual_buttonHeight - btnRadius) {
            UInt16 fromBottom = static_cast<UInt16>(
              actual_buttonHeight - 1 - row
            );
            Int16 bottomInset = CornerInset(btnRadius, fromBottom);

            if (bottomInset > leftInset) leftInset = bottomInset;
            if (bottomInset > rightInset) rightInset = bottomInset;
          }

          for (UInt16 col = 0; col < buttonWidth; ++col) {
            if (
              static_cast<Int16>(col) < leftInset
              || static_cast<Int16>(col) >= static_cast<Int16>(
                buttonWidth - rightInset
              )
            ) {
              continue;
            }

            UInt16 px = static_cast<UInt16>(buttonLeft + col);
            UInt16 py = static_cast<UInt16>(buttonTop + row);

            if (px >= _surfaceWidth || py >= _surfaceHeight) continue;

            UInt32 idx = py * _surfaceWidth + px;
            UInt8 srcAlpha = static_cast<UInt8>(
              (buttonColor >> 24) & 0xFF
            );

            _surface[idx] = Color::BlendOver(
              buttonColor, _surface[idx], srcAlpha
            );
          }
        }
      }

      if (button.Focused && Theme::DockButtonActiveBorderEnabled) {
        UInt32 borderColor = Theme::DockButtonActiveBorder;
        UInt8 borderR = static_cast<UInt8>((borderColor >> 16) & 0xFF);
        UInt8 borderG = static_cast<UInt8>((borderColor >> 8) & 0xFF);
        UInt8 borderB = static_cast<UInt8>(borderColor & 0xFF);
        UInt8 btnRadius = Theme::DockButtonRoundedCorners
          ? Theme::DockButtonBorderRadius
          : 0;

        for (UInt16 row = 0; row < actual_buttonHeight; ++row) {
          // top and bottom straight edges (outside corner regions)
          if (row == 0 || row == actual_buttonHeight - 1) {
            Int16 edgeInset = 0;

            if (row == 0 && btnRadius > 0) {
              edgeInset = Math::CornerInset(btnRadius, 0);
            } else if (row == actual_buttonHeight - 1 && btnRadius > 0) {
              edgeInset = Math::CornerInset(btnRadius, 0);
            }

            for (UInt16 col = static_cast<UInt16>(edgeInset);
                 col < buttonWidth - edgeInset; ++col
            ) {
              UInt16 px = static_cast<UInt16>(buttonLeft + col);
              UInt16 py = static_cast<UInt16>(buttonTop + row);

              if (px < _surfaceWidth && py < _surfaceHeight) {
                _surface[py * _surfaceWidth + px] = borderColor;
              }
            }

            continue;
          }

          // left and right straight edges (between corner regions)
          if (row >= btnRadius && row < actual_buttonHeight - btnRadius) {
            UInt16 py = static_cast<UInt16>(buttonTop + row);

            if (py < _surfaceHeight) {
              UInt16 leftPx = static_cast<UInt16>(buttonLeft);
              UInt16 rightPx = static_cast<UInt16>(
                buttonLeft + buttonWidth - 1
              );

              if (leftPx < _surfaceWidth) {
                _surface[py * _surfaceWidth + leftPx] = borderColor;
              }

              if (rightPx < _surfaceWidth) {
                _surface[py * _surfaceWidth + rightPx] = borderColor;
              }
            }

            continue;
          }

          // corner regions: AA border stroke
          if (btnRadius > 0) {
            bool isTop = row < btnRadius;
            UInt16 cornerRow = isTop
              ? row
              : static_cast<UInt16>(actual_buttonHeight - 1 - row);

            UInt8 innerRadius = (btnRadius > 1)
              ? static_cast<UInt8>(btnRadius - 1)
              : 0;

            // top/bottom edge of corner
            for (UInt16 localX = 0; localX < btnRadius; ++localX) {
              UInt8 outerCoverage = Math::CornerPixelCoverage(
                btnRadius, localX, cornerRow
              );

              if (outerCoverage == 0) continue;

              UInt8 innerCoverage = 0;

              if (innerRadius > 0 && localX >= 1 && cornerRow >= 1) {
                innerCoverage = Math::CornerPixelCoverage(
                  innerRadius,
                  static_cast<UInt16>(localX - 1),
                  static_cast<UInt16>(cornerRow - 1)
                );
              }

              UInt8 strokeAlpha = 0;

              if (outerCoverage > innerCoverage) {
                strokeAlpha = static_cast<UInt8>(
                  outerCoverage - innerCoverage
                );
              }

              if (strokeAlpha == 0) continue;

              // left corner
              UInt16 leftPx = static_cast<UInt16>(buttonLeft + localX);
              UInt16 py = static_cast<UInt16>(buttonTop + row);

              if (leftPx < _surfaceWidth && py < _surfaceHeight) {
                UInt32 idx = py * _surfaceWidth + leftPx;

                _surface[idx] = Color::BlendOver(
                  (static_cast<UInt32>(strokeAlpha) << 24)
                  | (static_cast<UInt32>(borderR) << 16)
                  | (static_cast<UInt32>(borderG) << 8)
                  | borderB,
                  _surface[idx],
                  strokeAlpha
                );
              }

              // right corner (mirrored)
              UInt16 rightPx = static_cast<UInt16>(
                buttonLeft + buttonWidth - 1 - localX
              );

              if (rightPx < _surfaceWidth && py < _surfaceHeight) {
                UInt32 idx = py * _surfaceWidth + rightPx;

                _surface[idx] = Color::BlendOver(
                  (static_cast<UInt32>(strokeAlpha) << 24)
                  | (static_cast<UInt32>(borderR) << 16)
                  | (static_cast<UInt32>(borderG) << 8)
                  | borderB,
                  _surface[idx],
                  strokeAlpha
                );
              }
            }
          }
        }
      }

      // draw a highlight strip along the top edge of the active button;
      // the strip starts at the dock's top chrome border row so the
      // active button visually replaces the WindowBorderColor line
      // within its width
      if (button.Focused && Theme::DockButtonTopHighlight) {
        UInt32 highlightColor = Theme::DockButtonTopHighlightColor;
        UInt8 thickness = Theme::DockButtonTopHighlightThickness;
        Int16 highlightTop = static_cast<Int16>(buttonTop - TopBorder);

        if (highlightTop < 0) highlightTop = 0;

        for (UInt8 layer = 0; layer < thickness; ++layer) {
          Int16 py = static_cast<Int16>(highlightTop + layer);

          if (py < 0 || py >= static_cast<Int16>(_surfaceHeight)) continue;

          for (UInt16 col = 0; col < buttonWidth; ++col) {
            UInt16 px = static_cast<UInt16>(buttonLeft + col);

            if (px >= _surfaceWidth) continue;

            _surface[static_cast<UInt32>(py) * _surfaceWidth + px]
              = highlightColor;
          }
        }
      }

      // draw title text (left-aligned with TextPadding); use the
      // active (bold) font for focused buttons, inactive (regular)
      // for unfocused ones
      const Fonts::BitmapFont* buttonFont =
        button.Focused ? _activeFont : _inactiveFont;

      if (buttonFont && button.Title[0] != '\0') {
        Int16 textX = static_cast<Int16>(buttonLeft + TextPadding);
        Int16 textY = static_cast<Int16>(
          buttonTop + 1 + buttonFont->CenterTextY(actual_buttonHeight - 2)
        );

        Canvas canvas(
          _surface,
          _surfaceWidth,
          _surfaceHeight,
          _surfaceWidth
        );

        Painter& painter = canvas.GetPainter();
        painter.SetFont(*buttonFont);

        UInt32 buttonBg = button.Focused
          ? Theme::DockButtonActiveBackground
          : Theme::DockBackground;

        UInt32 textColor = button.Focused
          ? Theme::DockButtonActiveText
          : Theme::DockButtonInactiveText;

        painter.DrawText(
          textX,
          textY,
          button.Title,
          textColor,
          buttonBg
        );
      }

      // draw trailing separator after each button (skip when full)
      if (ButtonSeparator && !_isFull) {
        Int16 separatorX = static_cast<Int16>(
          buttonLeft + buttonWidth + ButtonSpacing
        );

        if (separatorX >= 0 && separatorX < _surfaceWidth) {
          for (UInt16 row = 0; row < actual_buttonHeight; ++row) {
            UInt16 py = static_cast<UInt16>(buttonTop + row);

            if (py >= _surfaceHeight) break;

            for (UInt16 sw = 0; sw < ButtonSeparatorWidth; ++sw) {
              UInt16 px = static_cast<UInt16>(separatorX + sw);

              if (px >= _surfaceWidth) break;

              _surface[py * _surfaceWidth + px]
                = Theme::EnableTranslucency
                  ? Color::Composite(
                      Theme::DockButtonSeparator,
                      _surface[py * _surfaceWidth + px]
                    )
                  : (Theme::DockButtonSeparator | 0xFF000000);
            }
          }
        }
      }
    }
  }

  void Dock::DrawClipped(
    Rectangle clipRect,
    const DrawContext& ctx
  ) const {
    if (!_surface || !ctx.BlitBuffer) return;

    Rectangle dockFrame = GetFrame();
    Rectangle visible = dockFrame.Intersect(clipRect);

    if (visible.IsEmpty()) return;

    Int16 srcX = static_cast<Int16>(visible.Origin.X - dockFrame.Origin.X);
    Int16 srcY = static_cast<Int16>(visible.Origin.Y - dockFrame.Origin.Y);

    const UInt8* src
      = reinterpret_cast<const UInt8*>(_surface)
      + (static_cast<UInt32>(srcY) * _surfaceWidth + srcX) * 4;

    ctx.BlitBuffer(
      ctx.UserData,
      visible.Origin.X,
      visible.Origin.Y,
      src,
      visible.Dimensions.Width,
      visible.Dimensions.Height,
      _surfaceWidth,
      4
    );
  }
}
