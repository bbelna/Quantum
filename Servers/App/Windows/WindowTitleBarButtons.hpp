/**
 * @file Servers/App/Windows/WindowTitleBarButtons.hpp
 * @brief Declares the title bar button container.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "WindowTitleBarButton.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Manages a collection of @ref WindowTitleBarButton instances,
   *        laying them out right-to-left on the right side of the title
   *        bar.
   */
  class WindowTitleBarButtons {
    public:
      static constexpr UInt16 Gap = 6;

      WindowTitleBarButtons(Rectangle* windowFrame);

      ~WindowTitleBarButtons();

      void AddButton(WindowTitleBarButton* button);

      /**
       * @brief Sets the inset from the frame edge to the title bar
       *        content area and propagates to all child buttons.
       */
      void SetEdgeInset(UInt16 right, UInt16 top);

      bool Contains(Point p) const;

      WindowTitleBarButton* ButtonAt(Point p) const;

      /**
       * @brief Returns the x-coordinate of the leftmost button's left
       *        edge.
       */
      Int16 GetLeftEdge() const;

    private:
      Rectangle* _windowFrame;
      List<WindowTitleBarButton*> _buttons;
      UInt16 _edgeInsetRight = 0;
      UInt16 _edgeInsetTop = 0;
  };
}
