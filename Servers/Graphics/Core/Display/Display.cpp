/**
 * @file Servers/Graphics/Core/Display/Display.cpp
 * @brief Implements @ref @QGfxSrv::Display.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Fonts/DefaultFont.hpp>

#include "Display.hpp"
#include "../Text/GlyphRenderer.hpp"

namespace Quantum::Servers::Graphics {
  Display::Display(
    KernelClient& kernel,
    ServerLog& log,
    Cursor& cursor
  ) :
    backBuffer(kernel, log),
    _cursor(cursor)
  {
    font = &Fonts::DefaultFont;
  }

  Display::~Display() {
    delete driver;
  }

  // -------------------------------------------------------------------
  // Fanout draw operations
  // -------------------------------------------------------------------

  void Display::FillRectangle(
    UInt16 x, UInt16 y,
    UInt16 width, UInt16 height,
    UInt32 color
  ) {
    if (_batchMode) {
      backBuffer.FillRectangle(x, y, width, height, color);
      driver->FillRectangle(x, y, width, height, color);
      ExpandDirtyRectangle(x, y, width, height);
    } else {
      bool overlap
        = !hasHardwareCursor
       && _cursor.Overlaps(x, y, width, height);

      if (overlap) {
        _cursor.Undraw(*this);
      }

      backBuffer.FillRectangle(x, y, width, height, color);
      driver->FillRectangle(x, y, width, height, color);

      if (overlap) {
        _cursor.Draw(*this);
      }
    }
  }

  void Display::BlitBuffer(
    UInt16 x, UInt16 y,
    UInt16 width, UInt16 height,
    UInt32 transparent,
    const UInt32* pixels
  ) {
    if (_batchMode) {
      backBuffer.BlitBuffer(x, y, width, height, transparent, pixels);
      driver->BlitBuffer(x, y, width, height, transparent, pixels);
      ExpandDirtyRectangle(x, y, width, height);
    } else {
      bool overlap
        = !hasHardwareCursor
       && _cursor.Overlaps(x, y, width, height);

      if (overlap) {
        _cursor.Undraw(*this);
      }

      backBuffer.BlitBuffer(x, y, width, height, transparent, pixels);
      driver->BlitBuffer(x, y, width, height, transparent, pixels);

      if (overlap) {
        _cursor.Draw(*this);
      }
    }
  }

  void Display::XORRectangle(
    UInt16 x, UInt16 y,
    UInt16 width, UInt16 height,
    UInt32 color
  ) {
    if (_batchMode) {
      backBuffer.XORRectangle(x, y, width, height, color);
      driver->XORRectangle(x, y, width, height, color);
      ExpandDirtyRectangle(x, y, width, height);
    } else {
      bool overlap
        = !hasHardwareCursor
       && _cursor.Overlaps(x, y, width, height);

      if (overlap) {
        _cursor.Undraw(*this);
      }

      backBuffer.XORRectangle(x, y, width, height, color);
      driver->XORRectangle(x, y, width, height, color);

      if (overlap) {
        _cursor.Draw(*this);
      }
    }
  }

  void Display::DrawText(
    UInt16 x, UInt16 y,
    const char* text,
    Size textLength,
    UInt32 foreground,
    UInt32 background,
    bool hasBackground,
    bool bold
  ) {
    UInt16 totalWidth = font->TextWidth(text, textLength);
    UInt16 totalHeight = font->Height;

    if (_batchMode) {
      UInt16 penX = x;

      for (
        Size index = 0;
        index < textLength && text[index];
        index++
      ) {
        UInt8 code = static_cast<UInt8>(text[index]);

        DrawGlyph(
          *this, penX, y, code,
          foreground, background,
          hasBackground, bold
        );

        penX = static_cast<UInt16>(penX + font->GetAdvance(code));
      }

      ExpandDirtyRectangle(x, y, totalWidth, totalHeight);
    } else {
      bool overlap
        = !hasHardwareCursor
       && _cursor.Overlaps(x, y, totalWidth, totalHeight);

      if (overlap) {
        _cursor.Undraw(*this);
      }

      UInt16 penX = x;

      for (
        Size index = 0;
        index < textLength && text[index];
        index++
      ) {
        UInt8 code = static_cast<UInt8>(text[index]);

        DrawGlyph(
          *this, penX, y, code,
          foreground, background,
          hasBackground, bold
        );

        penX = static_cast<UInt16>(penX + font->GetAdvance(code));
      }

      if (overlap) {
        _cursor.Draw(*this);
      }
    }
  }

  // -------------------------------------------------------------------
  // Batch control
  // -------------------------------------------------------------------

  void Display::BeginBatch() {
    _dirtyX1 = _dirtyX2 = 0;
    _dirtyY1 = _dirtyY2 = 0;
    _batchMode = true;

    driver->SetBatchMode(true);
  }

  void Display::EndBatch() {
    if (_batchMode) {
      if (_dirtyX1 < _dirtyX2 && _dirtyY1 < _dirtyY2) {
        UInt16 dirtyWidth
          = static_cast<UInt16>(_dirtyX2 - _dirtyX1);
        UInt16 dirtyHeight
          = static_cast<UInt16>(_dirtyY2 - _dirtyY1);

        driver->FlushRegion(
          _dirtyX1, _dirtyY1,
          dirtyWidth, dirtyHeight
        );

        if (
          !hasHardwareCursor &&
          _cursor.IsVisible() &&
          _cursor.Overlaps(
            _dirtyX1, _dirtyY1,
            dirtyWidth, dirtyHeight
          )
        ) {
          _cursor.Draw(*this);
        }
      }

      _dirtyX1 = _dirtyX2 = 0;
      _dirtyY1 = _dirtyY2 = 0;
      _batchMode = false;

      driver->SetBatchMode(false);
    }
  }

  void Display::ExpandDirtyRectangle(
    UInt16 x, UInt16 y,
    UInt16 width, UInt16 height
  ) {
    UInt16 right = x + width;
    UInt16 bottom = y + height;

    if (right > screenWidth) {
      right = screenWidth;
    }

    if (bottom > screenHeight) {
      bottom = screenHeight;
    }

    if (_dirtyX1 == _dirtyX2) {
      _dirtyX1 = x;
      _dirtyY1 = y;
      _dirtyX2 = right;
      _dirtyY2 = bottom;
    } else {
      if (x < _dirtyX1) {
        _dirtyX1 = x;
      }

      if (y < _dirtyY1) {
        _dirtyY1 = y;
      }

      if (right > _dirtyX2) {
        _dirtyX2 = right;
      }

      if (bottom > _dirtyY2) {
        _dirtyY2 = bottom;
      }
    }
  }

}
