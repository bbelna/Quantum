/**
 * @file Servers/App/Windows/WindowTitleBarButton.hpp
 * @brief Declares @ref @QAppSrv::WindowTitleBarButton.
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
   * @brief A traffic-light circle button placed on the right side of a
   *        window's title bar.
   *
   * Each button computes its screen-space frame on the fly from the
   * owning window's frame pointer and its offset from the right edge.
   */
  class WindowTitleBarButton : public HasFrame {
    public:
      WindowTitleBarButton(
        Rectangle* windowFrame,
        UInt32 color,
        UInt32 pressedColor,
        UInt16 width,
        UInt16 height
      );

      Rectangle GetFrame() const override;

      UInt16 GetSize() const {
        return static_cast<UInt16>(_width + _marginLeft);
      }

      void SetMarginLeft(UInt16 margin) { _marginLeft = margin; }
      void SetOffsetFromRight(UInt16 offset) { _offsetFromRight = offset; }

      /**
       * @brief Sets the inset from the frame's right/top edges to
       *        the title bar content area. This is bezelTotal +
       *        titleBar padding on each axis.
       */
      void SetEdgeInset(UInt16 right, UInt16 top) {
        _edgeInsetRight = right;
        _edgeInsetTop = top;
      }

      /**
       * @brief Sets the height of the content area within the title
       *        bar (i.e., titleBarHeight - padding.Top - padding.Bottom)
       *        used for vertical centering.
       */
      void SetContentAreaHeight(UInt16 height) {
        _contentAreaHeight = height;
      }

      void SetPressed(bool pressed) { _pressed = pressed; }
      bool IsPressed() const { return _pressed; }
      void SetActive(bool active) { _active = active; }

    private:
      Rectangle* _windowFrame;
      UInt32 _color;
      UInt32 _pressedColor;
      UInt16 _width;
      UInt16 _height;
      UInt16 _offsetFromRight = 0;
      UInt16 _marginLeft = 0;
      UInt16 _edgeInsetRight = 0;
      UInt16 _edgeInsetTop = 0;
      UInt16 _contentAreaHeight = 12;
      bool _pressed = false;
      bool _active = true;
  };
}
