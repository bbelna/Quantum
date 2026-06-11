/**
 * @file UI/Border.cpp
 * @brief Implements @ref @QUI::Border.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/UI/Border.hpp>

#include "UITypes.hpp"

namespace Quantum::UI {
  void Border::Draw(
    Canvas& canvas,
    Rectangle contentBounds
  ) {
    Int16 cx = contentBounds.Origin.X;
    Int16 cy = contentBounds.Origin.Y;
    UInt16 cw = contentBounds.Dimensions.Width;
    UInt16 ch = contentBounds.Dimensions.Height;

    UInt8 top = Thicknesses.Top;
    UInt8 left = Thicknesses.Left;
    UInt8 right = Thicknesses.Right;
    UInt8 bottom = Thicknesses.Bottom;

    // top edge: spans full outer width, sits above content
    if (top > 0) {
      canvas.GetPainter().FillRectangle(
        Rectangle(
          static_cast<Int16>(cx - left),
          static_cast<Int16>(cy - top),
          static_cast<UInt16>(left + cw + right),
          top
        ),
        Colors.Top
      );
    }

    // bottom edge: spans full outer width, sits below content
    if (bottom > 0) {
      canvas.GetPainter().FillRectangle(
        Rectangle(
          static_cast<Int16>(cx - left),
          static_cast<Int16>(cy + ch),
          static_cast<UInt16>(left + cw + right),
          bottom
        ),
        Colors.Bottom
      );
    }

    // left edge: content height only (corners covered by top/bottom)
    if (left > 0) {
      canvas.GetPainter().FillRectangle(
        Rectangle(
          static_cast<Int16>(cx - left),
          cy,
          left,
          ch
        ),
        Colors.Left
      );
    }

    // right edge: content height only (corners covered by top/bottom)
    if (right > 0) {
      canvas.GetPainter().FillRectangle(
        Rectangle(
          static_cast<Int16>(cx + cw),
          cy,
          right,
          ch
        ),
        Colors.Right
      );
    }
  }

  Rectangle Border::ExpandBounds(
    Rectangle contentBounds
  ) const {
    return Rectangle(
      static_cast<Int16>(contentBounds.Origin.X - Thicknesses.Left),
      static_cast<Int16>(contentBounds.Origin.Y - Thicknesses.Top),
      static_cast<UInt16>(
        contentBounds.Dimensions.Width
        + Thicknesses.Left
        + Thicknesses.Right
      ),
      static_cast<UInt16>(
        contentBounds.Dimensions.Height
        + Thicknesses.Top
        + Thicknesses.Bottom
      )
    );
  }
}
