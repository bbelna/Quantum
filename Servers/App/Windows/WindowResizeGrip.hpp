/**
 * @file Servers/App/Windows/WindowResizeGrip.hpp
 * @brief Declares @ref @QAppSrv::WindowResizeGrip.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "WindowTheme.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Represents the resize grip in the bottom-right corner of a
   *        window. Provides both hit-test geometry (via @ref HasFrame)
   *        and chrome rendering (via @ref IDecoration).
   */
  class WindowResizeGrip : public HasFrame, public IDecoration {
    public:
      WindowResizeGrip(
        const ResizeGripTheme& theme,
        Rectangle* windowFrame
      );

      // ── HasFrame (hit testing) ────────────────────────────────────

      Rectangle GetFrame() const override;

      // ── IDecoration (chrome rendering) ────────────────────────────

      void Draw(Canvas& canvas, Rectangle contentRectangle) override;

      Rectangle ExpandBounds(
        Rectangle contentRectangle
      ) const override;

      void SetEnabled(bool enabled) { _enabled = enabled; }

    private:
      const ResizeGripTheme& _theme;
      Rectangle* _windowFrame;
      bool _enabled = true;
  };
}
