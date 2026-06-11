/**
 * @file Servers/Graphics/Core/Cursor/Cursor.cpp
 * @brief Implements @ref @QGfxSrv::Cursor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Cursor.hpp"

#include "../Display/BackBuffer.hpp"
#include "../Display/Display.hpp"

namespace Quantum::Servers::Graphics {
  Cursor::Cursor() = default;

  void Cursor::SetBitmap(
    UInt8 width,
    UInt8 height,
    UInt32 transparentColor,
    const UInt32* pixels
  ) {
    if (width > MaxCursorWidth) {
      width = MaxCursorWidth;
    }

    if (height > MaxCursorHeight) {
      height = MaxCursorHeight;
    }

    _width = width;
    _height = height;
    _transparentColor = transparentColor;

    for (UInt8 row = 0; row < height; ++row) {
      Byte::Copy(
        &_bitmap[row][0],
        &pixels[static_cast<Size>(row) * width],
        static_cast<Size>(width) * sizeof(UInt32)
      );
    }
  }

  bool Cursor::Overlaps(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height
  ) const {
    if (_visible && _width > 0 && _height > 0) {
      Int16 cursorRight = static_cast<Int16>(_x + _width);
      Int16 cursorBottom = static_cast<Int16>(_y + _height);
      Int16 rectRight = static_cast<Int16>(x + width);
      Int16 rectBottom = static_cast<Int16>(y + height);

      return _x < rectRight
          && cursorRight > static_cast<Int16>(x)
          && _y < rectBottom
          && cursorBottom > static_cast<Int16>(y);
    }

    return false;
  }

  Cursor::VisibleRect Cursor::ComputeVisibleRect(
    UInt16 screenWidth,
    UInt16 screenHeight
  ) const {
    VisibleRect rect = {};

    if (_width == 0 || _height == 0) {
      return rect;
    }

    UInt16 clampedX = static_cast<UInt16>(_x < 0 ? 0 : _x);
    UInt16 clampedY = static_cast<UInt16>(_y < 0 ? 0 : _y);

    UInt8 startCol = _x < 0 ? static_cast<UInt8>(-_x) : 0;
    UInt8 startRow = _y < 0 ? static_cast<UInt8>(-_y) : 0;
    UInt8 endCol = _width;
    UInt8 endRow = _height;

    if (clampedX + (endCol - startCol) > screenWidth) {
      endCol = static_cast<UInt8>(screenWidth - clampedX + startCol);
    }

    if (clampedY + (endRow - startRow) > screenHeight) {
      endRow = static_cast<UInt8>(screenHeight - clampedY + startRow);
    }

    UInt8 visibleWidth = endCol > startCol
      ? static_cast<UInt8>(endCol - startCol)
      : 0;
    UInt8 visibleHeight = endRow > startRow
      ? static_cast<UInt8>(endRow - startRow)
      : 0;

    rect.X = clampedX;
    rect.Y = clampedY;
    rect.VisibleWidth = visibleWidth;
    rect.VisibleHeight = visibleHeight;

    return rect;
  }

  void Cursor::Draw(Display& display) {
    if (!_visible || _width == 0 || _height == 0) {
      return;
    }

    BackBuffer& backBuffer = display.backBuffer;
    GraphicsDriverClient& driver = *display.driver;
    UInt16 screenWidth = display.screenWidth;
    UInt16 screenHeight = display.screenHeight;

    UInt32 composite[MaxCursorWidth * MaxCursorHeight];

    UInt16 clampedX = static_cast<UInt16>(_x < 0 ? 0 : _x);
    UInt16 clampedY = static_cast<UInt16>(_y < 0 ? 0 : _y);

    UInt8 startCol = _x < 0 ? static_cast<UInt8>(-_x) : 0;
    UInt8 startRow = _y < 0 ? static_cast<UInt8>(-_y) : 0;
    UInt8 endCol = _width;
    UInt8 endRow = _height;

    if (clampedX + (endCol - startCol) > screenWidth) {
      endCol = static_cast<UInt8>(screenWidth - clampedX + startCol);
    }

    if (clampedY + (endRow - startRow) > screenHeight) {
      endRow = static_cast<UInt8>(screenHeight - clampedY + startRow);
    }

    UInt8 visibleWidth = endCol - startCol;
    UInt8 visibleHeight = endRow - startRow;

    if (visibleWidth == 0 || visibleHeight == 0) {
      return;
    }

    for (UInt8 row = 0; row < visibleHeight; ++row) {
      for (UInt8 col = 0; col < visibleWidth; ++col) {
        UInt8 bitmapRow = startRow + row;
        UInt8 bitmapCol = startCol + col;
        UInt32 pixel = _bitmap[bitmapRow][bitmapCol];

        UInt16 screenX = clampedX + col;
        UInt16 screenY = clampedY + row;
        UInt32 index
          = static_cast<UInt32>(screenY) * screenWidth + screenX;

        UInt32 background;

        if (backBuffer.GetFormat() == PixelFormat::RGB565) {
          UInt16 nativePixel = static_cast<UInt16*>(
            backBuffer.GetBuffer()
          )[index];
          UInt32 red = (nativePixel >> 11) & 0x1F;
          UInt32 green = (nativePixel >> 5) & 0x3F;
          UInt32 blue = nativePixel & 0x1F;

          background = 0xFF000000
            | ((red << 3) | (red >> 2)) << 16
            | ((green << 2) | (green >> 4)) << 8
            | ((blue << 3) | (blue >> 2));
        } else {
          background = static_cast<UInt32*>(
            backBuffer.GetBuffer()
          )[index];
        }

        if (pixel == _transparentColor) {
          composite[row * visibleWidth + col] = background;
        } else {
          UInt8 alpha = static_cast<UInt8>((pixel >> 24) & 0xFF);

          if (alpha == 0xFF) {
            composite[row * visibleWidth + col] = pixel;
          } else {
            UInt8 inverseAlpha = static_cast<UInt8>(255 - alpha);
            UInt8 outRed = static_cast<UInt8>(
              (((pixel >> 16) & 0xFF) * alpha
              + ((background >> 16) & 0xFF) * inverseAlpha
              + 127) / 255
            );
            UInt8 outGreen = static_cast<UInt8>(
              (((pixel >> 8) & 0xFF) * alpha
              + ((background >> 8) & 0xFF) * inverseAlpha
              + 127) / 255
            );
            UInt8 outBlue = static_cast<UInt8>(
              ((pixel & 0xFF) * alpha
              + (background & 0xFF) * inverseAlpha
              + 127) / 255
            );

            composite[row * visibleWidth + col]
              = 0xFF000000
              | (static_cast<UInt32>(outRed) << 16)
              | (static_cast<UInt32>(outGreen) << 8)
              | static_cast<UInt32>(outBlue);
          }
        }
      }
    }

    driver.BlitBuffer(
      clampedX,
      clampedY,
      visibleWidth,
      visibleHeight,
      0x01000000,
      composite
    );
  }

  void Cursor::Undraw(Display& display) {
    if (!_visible || _width == 0 || _height == 0) {
      return;
    }

    BackBuffer& backBuffer = display.backBuffer;
    GraphicsDriverClient& driver = *display.driver;
    UInt16 screenWidth = display.screenWidth;
    UInt16 screenHeight = display.screenHeight;

    UInt16 clampedX = static_cast<UInt16>(_x < 0 ? 0 : _x);
    UInt16 clampedY = static_cast<UInt16>(_y < 0 ? 0 : _y);

    UInt8 startCol = _x < 0 ? static_cast<UInt8>(-_x) : 0;
    UInt8 startRow = _y < 0 ? static_cast<UInt8>(-_y) : 0;
    UInt8 endCol = _width;
    UInt8 endRow = _height;

    if (clampedX + (endCol - startCol) > screenWidth) {
      endCol = static_cast<UInt8>(screenWidth - clampedX + startCol);
    }

    if (clampedY + (endRow - startRow) > screenHeight) {
      endRow = static_cast<UInt8>(screenHeight - clampedY + startRow);
    }

    UInt8 visibleWidth = endCol - startCol;
    UInt8 visibleHeight = endRow - startRow;

    if (visibleWidth == 0 || visibleHeight == 0) {
      return;
    }

    UInt32 restore[MaxCursorWidth * MaxCursorHeight];

    if (backBuffer.GetFormat() == PixelFormat::RGB565) {
      UInt16* buffer = static_cast<UInt16*>(backBuffer.GetBuffer());

      for (UInt8 row = 0; row < visibleHeight; ++row) {
        UInt32 bufferRow
          = static_cast<UInt32>(clampedY + row) * screenWidth
          + clampedX;

        for (UInt8 col = 0; col < visibleWidth; ++col) {
          UInt16 nativePixel = buffer[bufferRow + col];
          UInt32 red = (nativePixel >> 11) & 0x1F;
          UInt32 green = (nativePixel >> 5) & 0x3F;
          UInt32 blue = nativePixel & 0x1F;

          restore[row * visibleWidth + col]
            = 0xFF000000
            | ((red << 3) | (red >> 2)) << 16
            | ((green << 2) | (green >> 4)) << 8
            | ((blue << 3) | (blue >> 2));
        }
      }
    } else {
      UInt32* buffer = static_cast<UInt32*>(backBuffer.GetBuffer());

      for (UInt8 row = 0; row < visibleHeight; ++row) {
        UInt32 bufferRow
          = static_cast<UInt32>(clampedY + row) * screenWidth
          + clampedX;

        Byte::Copy(
          &restore[row * visibleWidth],
          &buffer[bufferRow],
          static_cast<Size>(visibleWidth) * sizeof(UInt32)
        );
      }
    }

    driver.BlitBuffer(
      clampedX,
      clampedY,
      visibleWidth,
      visibleHeight,
      0x01000000,
      restore
    );
  }
}
