/**
 * @file UI/Bezel.cpp
 * @brief Implements @ref @QUI::Bezel.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/UI/Bezel.hpp>

#include "UITypes.hpp"

namespace Quantum::UI {
  BezelLayer& Bezel::AddLayer() {
    BezelLayer& layer = Layers[LayerCount++];

    layer = BezelLayer{};

    return layer;
  }

  BezelLayer& Bezel::AddLayer(UInt32 color, UInt8 thickness) {
    BezelLayer& layer = AddLayer();

    layer.Colors = { color, color, color, color };
    layer.Thickness = thickness;

    return layer;
  }

  BezelLayer& Bezel::AddLayer(
    UInt32 lightColor,
    UInt32 darkColor,
    UInt8 thickness
  ) {
    BezelLayer& layer = AddLayer();

    layer.Colors = {
      lightColor,   // Top
      lightColor,   // Left
      darkColor,    // Right
      darkColor     // Bottom
    };
    layer.Thickness = thickness;

    return layer;
  }

  UInt16 Bezel::GetTotalThickness() const {
    UInt16 total = 0;

    for (Size i = 0; i < LayerCount; ++i) {
      total = static_cast<UInt16>(total + Layers[i].Thickness);
    }

    return total;
  }

  void Bezel::Draw(
    Canvas& canvas,
    Rectangle contentRectangle
  ) {
    Painter& painter = canvas.GetPainter();

    // draw layers from outermost (index 0) inward, consuming
    // space from the edges of the given rectangle
    Int16 cx = contentRectangle.Origin.X;
    Int16 cy = contentRectangle.Origin.Y;
    UInt16 cw = contentRectangle.Dimensions.Width;
    UInt16 ch = contentRectangle.Dimensions.Height;

    for (Size i = 0; i < LayerCount; ++i) {
      const BezelLayer& layer = Layers[i];

      if (layer.Thickness == 0) continue;

      UInt8 t = layer.Thickness;

      // top strip: full current width
      painter.FillRectangle(
        Rectangle(cx, cy, cw, t),
        layer.Colors.Top
      );

      // bottom strip: full current width
      painter.FillRectangle(
        Rectangle(
          cx,
          static_cast<Int16>(cy + ch - t),
          cw,
          t
        ),
        layer.Colors.Bottom
      );

      // left strip: between top and bottom
      UInt16 innerH = static_cast<UInt16>(ch - 2 * t);

      if (innerH > 0) {
        painter.FillRectangle(
          Rectangle(
            cx,
            static_cast<Int16>(cy + t),
            t,
            innerH
          ),
          layer.Colors.Left
        );
      }

      // right strip: between top and bottom
      if (innerH > 0) {
        painter.FillRectangle(
          Rectangle(
            static_cast<Int16>(cx + cw - t),
            static_cast<Int16>(cy + t),
            t,
            innerH
          ),
          layer.Colors.Right
        );
      }

      // shrink inward for next layer
      cx = static_cast<Int16>(cx + t);
      cy = static_cast<Int16>(cy + t);
      cw = static_cast<UInt16>(cw - 2 * t);
      ch = static_cast<UInt16>(ch - 2 * t);
    }
  }

  Rectangle Bezel::ExpandBounds(
    Rectangle contentRectangle
  ) const {
    UInt16 total = GetTotalThickness();

    return Rectangle(
      static_cast<Int16>(contentRectangle.Origin.X - total),
      static_cast<Int16>(contentRectangle.Origin.Y - total),
      static_cast<UInt16>(
        contentRectangle.Dimensions.Width + 2 * total
      ),
      static_cast<UInt16>(
        contentRectangle.Dimensions.Height + 2 * total
      )
    );
  }
}
