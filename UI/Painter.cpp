/**
 * @file UI/Painter.cpp
 * @brief Implements @ref @QUI::Painter.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Theme.hpp>
#include <Quantum/UI/Canvas.hpp>

#include "UITypes.hpp"

namespace Quantum::UI {
  static const BitmapFont* _globalFont = nullptr;

  Painter::Painter(Canvas& canvas) :
    _canvas(&canvas),
    _font(*GetDefaultFont())
  {
  }

  void Painter::SetFont(const BitmapFont& font) {
    _font = font;
  }

  void Painter::SetDefaultFont(const BitmapFont* font) {
    _globalFont = font;
  }

  const BitmapFont* Painter::GetDefaultFont() {
    return _globalFont ? _globalFont : &DefaultFont;
  }

  void Painter::FillRectangle(Rectangle rectangle, UInt32 color) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();
    UInt16 stride = _canvas->GetStride();
    UInt8 bytesPerPixel = _canvas->GetBytesPerPixel();

    if (!pixels) return;

    Rectangle bounds(0, 0, width, height);
    Rectangle clamped = rectangle.Intersect(bounds);

    if (clamped.IsEmpty()) return;

    Int16 x = clamped.Origin.X;
    Int16 y = clamped.Origin.Y;
    UInt16 fillWidth = clamped.Dimensions.Width;
    Int16 clampedBottomY = clamped.GetBottom();

    if (bytesPerPixel == 2) {
      UInt16 colorRGB565 = Canvas::ToRGB565(color);
      UInt32 packedColorRGB565
        = static_cast<UInt32>(colorRGB565)
        | (static_cast<UInt32>(colorRGB565) << 16);
      UInt16* buffer = static_cast<UInt16*>(pixels);

      for (Int16 row = y; row < clampedBottomY; ++row) {
        UInt16* destination = &buffer[static_cast<UInt32>(row) * stride + x];
        UInt16 pairs = fillWidth >> 1;
        UInt32* destination32 = reinterpret_cast<UInt32*>(destination);

        for (UInt16 p = 0; p < pairs; ++p) destination32[p] = packedColorRGB565;

        UInt16 column = static_cast<UInt16>(pairs << 1);

        if (column < fillWidth) destination[column] = colorRGB565;
      }
    } else {
      UInt32* buffer = static_cast<UInt32*>(pixels);

      for (Int16 row = y; row < clampedBottomY; ++row) {
        void* destination = &buffer[
          static_cast<UInt32>(row) * stride + x
        ];
        UInt32 count = fillWidth;

        asm volatile(
          "cld\n"
          "rep stosl"
          : "+D"(destination), "+c"(count)
          : "a"(color)
          : "memory"
        );
      }
    }
  }


  /**
   * @brief Computes coverage (0-255) for a single pixel in a rounded
   *        corner region.  When `Theme::EnableBorderCornerAA` is true
   *        the value is smooth (sub-pixel alpha); when false, coverage
   *        is clamped to binary 0 or 255 for hard-edged corners.
   * @param radius The corner radius in pixels.
   * @param localX Pixel column from the corner edge (0 = outermost).
   * @param localY Pixel row from the corner edge (0 = outermost).
   * @return Coverage value: 255 = fully inside, 0 = fully outside.
   */
  static UInt8 CornerPixelCoverage(
    UInt8 radius, UInt16 localX, UInt16 localY
  ) {
    UInt8 coverage = Math::CornerPixelCoverage(
      radius, localX, localY
    );

    if constexpr (!Theme::EnableBorderCornerAA) {
      return coverage >= 128 ? 255 : 0;
    }

    return coverage;
  }

  void Painter::FillRoundedRectangle(
    Rectangle rectangle,
    UInt32 color,
    UInt8 radius,
    RoundedCorners corners
  ) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();
    UInt16 stride = _canvas->GetStride();
    UInt8 bytesPerPixel = _canvas->GetBytesPerPixel();

    if (!pixels) return;

    if (radius == 0 || corners == RoundedCorners::None) {
      FillRectangle(rectangle, color);

      return;
    }

    Rectangle bounds(0, 0, width, height);
    Rectangle clamped = rectangle.Intersect(bounds);

    if (clamped.IsEmpty()) return;

    UInt16 halfWidth = static_cast<UInt16>(rectangle.Dimensions.Width / 2);
    UInt16 halfHeight = static_cast<UInt16>(rectangle.Dimensions.Height / 2);
    UInt16 maxRadius = halfWidth < halfHeight ? halfWidth : halfHeight;

    if (radius > maxRadius) radius = static_cast<UInt8>(maxRadius);

    bool roundTopLeft = Enum::HasFlag(corners, RoundedCorners::TopLeft);
    bool roundTopRight = Enum::HasFlag(corners, RoundedCorners::TopRight);
    bool roundBottomLeft = Enum::HasFlag(corners, RoundedCorners::BottomLeft);
    bool roundBottomRight = Enum::HasFlag(corners, RoundedCorners::BottomRight);

    Int16 rectLeft = rectangle.Origin.X;
    Int16 rectTop = rectangle.Origin.Y;
    Int16 rectRight = rectangle.GetRight();
    Int16 rectBottom = rectangle.GetBottom();

    UInt8 fillAlpha = static_cast<UInt8>((color >> 24) & 0xFF);
    UInt16 colorRGB565 = (bytesPerPixel == 2) ? Canvas::ToRGB565(color) : 0;

    for (Int16 row = clamped.Origin.Y; row < clamped.GetBottom(); ++row) {
      bool inTopZone = row < rectTop + radius;
      bool inBottomZone = row >= rectBottom - radius;

      UInt16 localY = 0;

      if (inTopZone) {
        localY = static_cast<UInt16>(row - rectTop);
      } else if (inBottomZone) {
        localY = static_cast<UInt16>(rectBottom - 1 - row);
      }

      bool hasLeftCorner
        = (inTopZone && roundTopLeft)
        || (inBottomZone && roundBottomLeft);
      bool hasRightCorner
        = (inTopZone && roundTopRight)
        || (inBottomZone && roundBottomRight);

      if (hasLeftCorner || hasRightCorner) {
        // per-pixel AA path: process full corner quadrants, then
        // fill the safe middle span between them

        // left corner quadrant: rectLeft .. rectLeft+radius
        if (hasLeftCorner) {
          Int16 cornerEnd = static_cast<Int16>(rectLeft + radius);

          for (Int16 px = rectLeft; px < cornerEnd; ++px) {
            if (px < clamped.Origin.X) continue;
            if (px >= clamped.GetRight()) break;

            UInt16 localX = static_cast<UInt16>(px - rectLeft);
            UInt8 coverage = CornerPixelCoverage(
              radius, localX, localY
            );

            if (coverage == 0) continue;

            UInt32 offset = static_cast<UInt32>(row) * stride + px;

            if (bytesPerPixel == 2) {
              UInt16* buffer = static_cast<UInt16*>(pixels);

              if (coverage == 255) {
                buffer[offset] = colorRGB565;
              } else {
                UInt8 alpha = static_cast<UInt8>(
                  (static_cast<UInt16>(fillAlpha) * coverage + 127)
                    / 255
                );

                buffer[offset] = Color::BlendRGB565(
                  colorRGB565, buffer[offset], alpha
                );
              }
            } else {
              UInt32* buffer = static_cast<UInt32*>(pixels);

              if (coverage == 255) {
                buffer[offset] = color;
              } else {
                UInt8 alpha = static_cast<UInt8>(
                  (static_cast<UInt16>(fillAlpha) * coverage + 127)
                    / 255
                );

                buffer[offset] = Color::BlendOver(
                  color, buffer[offset], alpha
                );
              }
            }
          }
        }

        // solid middle span (between corner quadrants)
        Int16 midLeft = static_cast<Int16>(
          rectLeft + (hasLeftCorner ? radius : 0)
        );
        Int16 midRight = static_cast<Int16>(
          rectRight - (hasRightCorner ? radius : 0)
        );

        if (midLeft < clamped.Origin.X) midLeft = clamped.Origin.X;

        if (midRight > clamped.GetRight()) {
          midRight = clamped.GetRight();
        }

        if (midLeft < midRight) {
          UInt16 midWidth = static_cast<UInt16>(midRight - midLeft);

          if (bytesPerPixel == 2) {
            UInt16* buffer = static_cast<UInt16*>(pixels);
            UInt16* destination = &buffer[
              static_cast<UInt32>(row) * stride + midLeft
            ];

            for (UInt16 col = 0; col < midWidth; ++col) {
              destination[col] = colorRGB565;
            }
          } else {
            UInt32* buffer = static_cast<UInt32*>(pixels);
            void* destination = &buffer[
              static_cast<UInt32>(row) * stride + midLeft
            ];
            UInt32 count = midWidth;

            asm volatile(
              "cld\n"
              "rep stosl"
              : "+D"(destination), "+c"(count)
              : "a"(color)
              : "memory"
            );
          }
        }

        // right corner quadrant: rectRight-radius .. rectRight
        if (hasRightCorner) {
          Int16 cornerStart = static_cast<Int16>(
            rectRight - radius
          );

          for (Int16 px = cornerStart; px < rectRight; ++px) {
            if (px < clamped.Origin.X) continue;
            if (px >= clamped.GetRight()) break;

            UInt16 localX = static_cast<UInt16>(
              rectRight - 1 - px
            );
            UInt8 coverage = CornerPixelCoverage(
              radius, localX, localY
            );

            if (coverage == 0) continue;

            UInt32 offset = static_cast<UInt32>(row) * stride + px;

            if (bytesPerPixel == 2) {
              UInt16* buffer = static_cast<UInt16*>(pixels);

              if (coverage == 255) {
                buffer[offset] = colorRGB565;
              } else {
                UInt8 alpha = static_cast<UInt8>(
                  (static_cast<UInt16>(fillAlpha) * coverage + 127)
                    / 255
                );

                buffer[offset] = Color::BlendRGB565(
                  colorRGB565, buffer[offset], alpha
                );
              }
            } else {
              UInt32* buffer = static_cast<UInt32*>(pixels);

              if (coverage == 255) {
                buffer[offset] = color;
              } else {
                UInt8 alpha = static_cast<UInt8>(
                  (static_cast<UInt16>(fillAlpha) * coverage + 127)
                    / 255
                );

                buffer[offset] = Color::BlendOver(
                  color, buffer[offset], alpha
                );
              }
            }
          }
        }
      } else {
        // no corners on this row - solid fill
        if (bytesPerPixel == 2) {
          UInt16* buffer = static_cast<UInt16*>(pixels);
          UInt16* destination = &buffer[
            static_cast<UInt32>(row) * stride + clamped.Origin.X
          ];
          UInt16 lineWidth = clamped.Dimensions.Width;

          for (UInt16 col = 0; col < lineWidth; ++col) {
            destination[col] = colorRGB565;
          }
        } else {
          UInt32* buffer = static_cast<UInt32*>(pixels);
          void* destination = &buffer[
            static_cast<UInt32>(row) * stride + clamped.Origin.X
          ];
          UInt32 count = clamped.Dimensions.Width;

          asm volatile(
            "cld\n"
            "rep stosl"
            : "+D"(destination), "+c"(count)
            : "a"(color)
            : "memory"
          );
        }
      }
    }
  }

  /**
   * @brief Computes the distance from a point to the nearest edge of a
   *        rounded rectangle. Returns 0 for points inside, positive for
   *        points outside.
   * @param px Pixel X coordinate (in 128ths).
   * @param py Pixel Y coordinate (in 128ths).
   * @param rectLeft Left edge (in 128ths).
   * @param rectTop Top edge (in 128ths).
   * @param rectRight Right edge (in 128ths).
   * @param rectBottom Bottom edge (in 128ths).
   * @param cornerR Corner radius (in 128ths).
   * @return Distance in 128ths, or 0 if inside.
   */
  static UInt32 DistanceToRoundedRect(
    Int32 px, Int32 py,
    Int32 rectLeft, Int32 rectTop,
    Int32 rectRight, Int32 rectBottom,
    Int32 cornerR
  ) {
    // clamp to the inner rectangle (inset by corner radius)
    Int32 innerLeft = rectLeft + cornerR;
    Int32 innerRight = rectRight - cornerR;
    Int32 innerTop = rectTop + cornerR;
    Int32 innerBottom = rectBottom - cornerR;

    // horizontal and vertical distances to the inner rect
    Int32 dx = 0;

    if (px < innerLeft) {
      dx = innerLeft - px;
    } else if (px > innerRight) {
      dx = px - innerRight;
    }

    Int32 dy = 0;

    if (py < innerTop) {
      dy = innerTop - py;
    } else if (py > innerBottom) {
      dy = py - innerBottom;
    }

    if (dx == 0 && dy == 0) {
      // inside the inner rect: definitely inside the rounded rect
      return 0;
    }

    if (dx > 0 && dy > 0) {
      // in a corner region: distance to the corner circle
      UInt32 dist2 = static_cast<UInt32>(dx) * static_cast<UInt32>(dx)
        + static_cast<UInt32>(dy) * static_cast<UInt32>(dy);
      UInt32 dist = Math::IntegerSqrt(dist2);

      return (dist > static_cast<UInt32>(cornerR))
        ? static_cast<UInt32>(dist - cornerR) : 0;
    }

    // on a straight edge: distance to the outer rect edge is
    // (distance to inner rect) minus cornerR
    Int32 edgeDist = (dx > dy ? dx : dy) - cornerR;

    return (edgeDist > 0) ? static_cast<UInt32>(edgeDist) : 0;
  }

  void Painter::RenderShadow(
    Rectangle contentRect,
    UInt8 shadowSize,
    UInt8 peakAlpha,
    UInt8 cornerRadius,
    Int16 offsetX,
    Int16 offsetY,
    RoundedCorners corners
  ) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();
    UInt16 stride = _canvas->GetStride();

    if (shadowSize == 0 || !pixels) return;

    // the shadow gradient is rendered around the shifted content rect;
    // on the offset side the effective radius is shadowSize + offset
    // so the gradient reaches the chrome edge; on the opposite side
    // it is just shadowSize (hidden behind chrome)
    Int16 shiftedX = static_cast<Int16>(
      contentRect.Origin.X + offsetX
    );
    Int16 shiftedY = static_cast<Int16>(
      contentRect.Origin.Y + offsetY
    );

    Int16 extendLeft = static_cast<Int16>(
      shadowSize + (offsetX < 0 ? -offsetX : 0)
    );
    Int16 extendRight = static_cast<Int16>(
      shadowSize + (offsetX > 0 ? offsetX : 0)
    );
    Int16 extendTop = static_cast<Int16>(
      shadowSize + (offsetY < 0 ? -offsetY : 0)
    );
    Int16 extendBottom = static_cast<Int16>(
      shadowSize + (offsetY > 0 ? offsetY : 0)
    );

    Int16 shadowLeft = static_cast<Int16>(shiftedX - extendLeft);
    Int16 shadowTop = static_cast<Int16>(shiftedY - extendTop);
    Int16 shadowRight = static_cast<Int16>(
      shiftedX + contentRect.Dimensions.Width + extendRight
    );
    Int16 shadowBottom = static_cast<Int16>(
      shiftedY + contentRect.Dimensions.Height + extendBottom
    );

    // clamp to surface bounds
    if (shadowLeft < 0) shadowLeft = 0;
    if (shadowTop < 0) shadowTop = 0;

    if (shadowRight > static_cast<Int16>(width)) {
      shadowRight = static_cast<Int16>(width);
    }

    if (shadowBottom > static_cast<Int16>(height)) {
      shadowBottom = static_cast<Int16>(height);
    }

    // shifted content rect in 128ths - this is the shadow's center
    Int32 crLeft128 = static_cast<Int32>(shiftedX) << 7;
    Int32 crTop128 = static_cast<Int32>(shiftedY) << 7;
    Int32 crRight128 = crLeft128
      + (static_cast<Int32>(contentRect.Dimensions.Width) << 7);
    Int32 crBottom128 = crTop128
      + (static_cast<Int32>(contentRect.Dimensions.Height) << 7);

    Int32 cr128 = static_cast<Int32>(cornerRadius) << 7;
    Int32 shadowSize128 = static_cast<Int32>(shadowSize) << 7;
    Int32 offsetX128 = static_cast<Int32>(offsetX) << 7;
    Int32 offsetY128 = static_cast<Int32>(offsetY) << 7;

    UInt32* buf = static_cast<UInt32*>(pixels);

    for (Int16 py = shadowTop; py < shadowBottom; ++py) {
      Int32 py128 = (static_cast<Int32>(py) << 7) + 64;

      for (Int16 px = shadowLeft; px < shadowRight; ++px) {
        Int32 px128 = (static_cast<Int32>(px) << 7) + 64;

        UInt32 dist = DistanceToRoundedRect(
          px128, py128,
          crLeft128, crTop128, crRight128, crBottom128,
          cr128
        );

        if (dist == 0) continue;

        // compute per-side effective radius: on the offset side the
        // shadow extends further so the gradient reaches the chrome;
        // minimum is shadowSize (opposite side), maximum is
        // shadowSize + abs(offset) (offset side)
        Int32 dxSign = 0;
        Int32 dySign = 0;

        if (px128 < crLeft128) {
          dxSign = -1;
        } else if (px128 > crRight128) {
          dxSign = 1;
        }

        if (py128 < crTop128) {
          dySign = -1;
        } else if (py128 > crBottom128) {
          dySign = 1;
        }

        Int32 effectiveRadius128
          = shadowSize128
          + dxSign * offsetX128
          + dySign * offsetY128;

        if (effectiveRadius128 < shadowSize128) {
          effectiveRadius128 = shadowSize128;
        }

        if (dist >= static_cast<UInt32>(effectiveRadius128)) {
          continue;
        }

        // smooth quadratic falloff over the full effective radius
        UInt32 t
          = static_cast<UInt32>(effectiveRadius128) - dist;
        UInt32 alpha
          = static_cast<UInt32>(peakAlpha) * t
          / static_cast<UInt32>(effectiveRadius128);
        alpha = alpha * t
          / static_cast<UInt32>(effectiveRadius128);

        if (alpha == 0) continue;
        if (alpha > 255) alpha = 255;

        // 2x2 ordered dither on the alpha channel so that the shadow
        // gradient produces visible intermediate steps on 16bpp
        // displays where 5-bit quantization would otherwise collapse
        // the entire gradient into a single hard step
        static constexpr UInt8 bayerThreshold[4] = { 0, 32, 48, 16 };
        UInt8 threshold = bayerThreshold[
          (static_cast<UInt8>(py) & 1) * 2
          + (static_cast<UInt8>(px) & 1)
        ];

        if (alpha < threshold) continue;

        UInt32 pixelOffset
          = static_cast<UInt32>(py) * stride
          + static_cast<UInt32>(px);

        buf[pixelOffset] = static_cast<UInt32>(alpha) << 24;
      }
    }
  }

  void Painter::DrawGlyph(
    UInt16 x,
    UInt16 y,
    char ch,
    UInt32 fg,
    UInt32 bg,
    bool bold
  ) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();
    UInt16 stride = _canvas->GetStride();
    UInt8 bytesPerPixel = _canvas->GetBytesPerPixel();

    if (!pixels) return;

    UInt8 code = static_cast<UInt8>(ch);
    UInt8 gw = _font.Width;
    UInt8 drawWidth = _font.GetAdvance(code);
    UInt8 gh = _font.Height;
    bool antialiased = _font.BitsPerPixel >= 8;
    bool lcd = _font.IsLCD();

    if (bytesPerPixel == 2) {
      UInt16 fg16 = Canvas::ToRGB565(fg);
      UInt16 bg16 = Canvas::ToRGB565(bg);
      UInt16 colors[2] = { bg16, fg16 };
      UInt16* buf = static_cast<UInt16*>(pixels);

      if (
        drawWidth == 8 && gw == 8
        && x + 8 <= width && y + gh <= height
      ) {
        for (UInt8 row = 0; row < gh; ++row) {
          UInt16* dst = &buf[
            static_cast<UInt32>(y + row) * stride + x
          ];
          UInt8 bits = _font.GetRow(code, row);

          dst[0] = colors[(bits >> 7) & 1];
          dst[1] = colors[(bits >> 6) & 1];
          dst[2] = colors[(bits >> 5) & 1];
          dst[3] = colors[(bits >> 4) & 1];
          dst[4] = colors[(bits >> 3) & 1];
          dst[5] = colors[(bits >> 2) & 1];
          dst[6] = colors[(bits >> 1) & 1];
          dst[7] = colors[bits & 1];
        }
      } else {
        for (UInt8 row = 0; row < gh; ++row) {
          UInt16 py = static_cast<UInt16>(y + row);

          if (py >= height) break;

          for (UInt8 col = 0; col < drawWidth; ++col) {
            UInt16 px = static_cast<UInt16>(x + col);

            if (px >= width) break;

            if (lcd) {
              UInt8 covR, covG, covB;
              _font.GetLCDCoverage(
                code, row, col, covR, covG, covB
              );

              buf[static_cast<UInt32>(py) * stride + px]
                = Color::BlendSubpixelRGB565(
                    fg16, bg16, covR, covG, covB
                  );
            } else if (antialiased) {
              UInt8 alpha = _font.GetAlpha(code, row, col);

              buf[static_cast<UInt32>(py) * stride + px]
                = Color::BlendGammaRGB565(fg16, bg16, alpha);
            } else {
              buf[static_cast<UInt32>(py) * stride + px]
                = colors[_font.GetPixel(code, row, col) ? 1 : 0];
            }
          }

          // draw overhang columns past the advance (blend only
          // non-zero pixels to avoid clobbering adjacent glyphs)
          for (UInt8 col = drawWidth; col < gw; ++col) {
            UInt16 px = static_cast<UInt16>(x + col);

            if (px >= width) break;

            if (antialiased) {
              if (lcd) {
                UInt8 covR, covG, covB;
                _font.GetLCDCoverage(
                  code, row, col, covR, covG, covB
                );

                if (covR == 0 && covG == 0 && covB == 0) continue;

                UInt16 existing = buf[
                  static_cast<UInt32>(py) * stride + px
                ];

                buf[static_cast<UInt32>(py) * stride + px]
                  = Color::BlendSubpixelRGB565(
                      fg16, existing, covR, covG, covB
                    );
              } else {
                UInt8 alpha = _font.GetAlpha(code, row, col);

                if (alpha == 0) continue;

                UInt16 existing = buf[
                  static_cast<UInt32>(py) * stride + px
                ];

                buf[static_cast<UInt32>(py) * stride + px]
                  = Color::BlendGammaRGB565(
                      fg16, existing, alpha
                    );
              }
            } else {
              if (_font.GetPixel(code, row, col)) {
                buf[static_cast<UInt32>(py) * stride + px] = fg16;
              }
            }
          }

          #if QUANTUM_UI_SYNTHETIC_BOLD_ENABLED
          if (bold) {
            for (UInt8 col = 0; col < drawWidth; ++col) {
              if (!_font.GetPixel(code, row, col)) continue;

              UInt16 px = static_cast<UInt16>(x + col + 1);

              if (px < width) {
                buf[static_cast<UInt32>(py) * stride + px] = colors[1];
              }
            }
          }
          #endif
        }
      }
    } else {
      UInt32 colors[2] = { bg, fg };
      UInt32* buf = static_cast<UInt32*>(pixels);

      if (
        drawWidth == 8 && gw == 8
        && x + 8 <= width && y + gh <= height
      ) {
        for (UInt8 row = 0; row < gh; ++row) {
          UInt32* dst = &buf[
            static_cast<UInt32>(y + row) * stride + x
          ];
          UInt8 bits = _font.GetRow(code, row);

          dst[0] = colors[(bits >> 7) & 1];
          dst[1] = colors[(bits >> 6) & 1];
          dst[2] = colors[(bits >> 5) & 1];
          dst[3] = colors[(bits >> 4) & 1];
          dst[4] = colors[(bits >> 3) & 1];
          dst[5] = colors[(bits >> 2) & 1];
          dst[6] = colors[(bits >> 1) & 1];
          dst[7] = colors[bits & 1];
        }
      } else {
        for (UInt8 row = 0; row < gh; ++row) {
          UInt16 py = static_cast<UInt16>(y + row);

          if (py >= height) break;

          // draw columns within the advance (fill bg for empty pixels)
          bool transparentBg = ((bg >> 24) & 0xFF) < 0xFF;

          for (UInt8 col = 0; col < drawWidth; ++col) {
            UInt16 px = static_cast<UInt16>(x + col);

            if (px >= width) break;

            if (lcd) {
              UInt8 covR, covG, covB;
              _font.GetLCDCoverage(
                code, row, col, covR, covG, covB
              );

              if (transparentBg) {
                if (covR == 0 && covG == 0 && covB == 0) continue;

                UInt32& dest = buf[
                  static_cast<UInt32>(py) * stride + px
                ];

                // Fall back to gamma-correct scalar blend on
                // translucent surfaces (subpixel fringing is
                // unacceptable on non-opaque backgrounds)
                UInt8 alpha = _font.GetAlpha(code, row, col);
                dest = Color::BlendGammaOver(fg, dest, alpha);
              } else {
                buf[static_cast<UInt32>(py) * stride + px]
                  = Color::BlendSubpixelARGB(
                      fg, bg, covR, covG, covB
                    );
              }
            } else if (antialiased) {
              UInt8 alpha = _font.GetAlpha(code, row, col);

              if (transparentBg) {
                if (alpha == 0) continue;

                UInt32& dest = buf[
                  static_cast<UInt32>(py) * stride + px
                ];

                dest = Color::BlendGammaOver(fg, dest, alpha);
              } else {
                buf[static_cast<UInt32>(py) * stride + px]
                  = Color::BlendGammaARGB(fg, bg, alpha);
              }
            } else {
              if (transparentBg && !_font.GetPixel(code, row, col)) {
                continue;
              }

              buf[static_cast<UInt32>(py) * stride + px]
                = colors[_font.GetPixel(code, row, col) ? 1 : 0];
            }
          }

          // draw overhang columns past the advance (blend only
          // non-zero pixels to avoid clobbering adjacent glyphs)
          for (UInt8 col = drawWidth; col < gw; ++col) {
            UInt16 px = static_cast<UInt16>(x + col);

            if (px >= width) break;

            if (antialiased) {
              if (lcd) {
                UInt8 covR, covG, covB;
                _font.GetLCDCoverage(
                  code, row, col, covR, covG, covB
                );

                if (covR == 0 && covG == 0 && covB == 0) continue;

                UInt32& dest = buf[
                  static_cast<UInt32>(py) * stride + px
                ];

                if (transparentBg) {
                  UInt8 alpha = _font.GetAlpha(code, row, col);
                  dest = Color::BlendGammaOver(fg, dest, alpha);
                } else {
                  dest = Color::BlendSubpixelARGB(
                    fg, dest, covR, covG, covB
                  );
                }
              } else {
                UInt8 alpha = _font.GetAlpha(code, row, col);

                if (alpha == 0) continue;

                UInt32& dest = buf[
                  static_cast<UInt32>(py) * stride + px
                ];

                dest = transparentBg
                  ? Color::BlendGammaOver(fg, dest, alpha)
                  : Color::BlendGammaARGB(fg, dest, alpha);
              }
            } else {
              if (_font.GetPixel(code, row, col)) {
                buf[static_cast<UInt32>(py) * stride + px] = fg;
              }
            }
          }

          #if QUANTUM_UI_SYNTHETIC_BOLD_ENABLED
          if (bold && !antialiased) {
            for (UInt8 col = 0; col < drawWidth; ++col) {
              if (!_font.GetPixel(code, row, col)) continue;

              UInt16 px = static_cast<UInt16>(x + col + 1);

              if (px < width) {
                buf[static_cast<UInt32>(py) * stride + px] = colors[1];
              }
            }
          }
          #endif
        }
      }
    }
  }

  void Painter::DrawText(
    Int16 x,
    Int16 y,
    const char* text,
    UInt32 fg,
    UInt32 bg,
    bool bold
  ) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();

    if (!pixels || !text) return;

    Int16 cx = x;

    for (Size i = 0; text[i]; ++i) {
      UInt8 code = static_cast<UInt8>(text[i]);
      UInt8 adv = _font.GetAdvance(code);

      if (adv == 0) break;

      if (cx >= static_cast<Int16>(width)) break;

      if (cx + adv > 0) {
        DrawGlyph(
          static_cast<UInt16>(cx),
          static_cast<UInt16>(y),
          text[i],
          fg,
          bg,
          bold
        );
      }

      cx = static_cast<Int16>(cx + adv);
    }
  }

  void Painter::Clear(UInt32 color) {
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();

    FillRectangle(Rectangle(0, 0, width, height), color);
  }

  void Painter::ScrollUp(UInt16 rows, UInt32 fillColor) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();
    UInt16 stride = _canvas->GetStride();
    UInt8 bytesPerPixel = _canvas->GetBytesPerPixel();

    if (!pixels || rows == 0 || rows >= height) return;

    UInt16 remaining = static_cast<UInt16>(height - rows);

    if (bytesPerPixel == 2) {
      UInt16* buf = static_cast<UInt16*>(pixels);
      void* dst = buf;
      const void* src = &buf[static_cast<UInt32>(rows) * stride];
      UInt32 count = static_cast<UInt32>(remaining) * stride;

      asm volatile(
        "cld\n"
        "rep movsw"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
      );
    } else {
      UInt32* buf = static_cast<UInt32*>(pixels);
      void* dst = buf;
      const void* src = &buf[static_cast<UInt32>(rows) * stride];
      UInt32 count = static_cast<UInt32>(remaining) * stride;

      asm volatile(
        "cld\n"
        "rep movsl"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
      );
    }

    FillRectangle(
      Rectangle(
        0,
        static_cast<Int16>(remaining),
        width,
        rows
      ),
      fillColor
    );
  }

  void Painter::ScrollDown(UInt16 rows, UInt32 fillColor) {
    void* pixels = _canvas->GetPixels();
    UInt16 width = _canvas->GetWidth();
    UInt16 height = _canvas->GetHeight();
    UInt16 stride = _canvas->GetStride();
    UInt8 bytesPerPixel = _canvas->GetBytesPerPixel();

    if (!pixels || rows == 0 || rows >= height) return;

    UInt16 remaining = static_cast<UInt16>(height - rows);

    if (bytesPerPixel == 2) {
      UInt16* buf = static_cast<UInt16*>(pixels);
      void* dst = &buf[static_cast<UInt32>(height) * stride - 1];
      const void* src = &buf[static_cast<UInt32>(remaining) * stride - 1];
      UInt32 count = static_cast<UInt32>(remaining) * stride;

      asm volatile(
        "std\n"
        "rep movsw\n"
        "cld"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
      );
    } else {
      UInt32* buf = static_cast<UInt32*>(pixels);
      void* dst = &buf[static_cast<UInt32>(height) * stride - 1];
      const void* src = &buf[static_cast<UInt32>(remaining) * stride - 1];
      UInt32 count = static_cast<UInt32>(remaining) * stride;

      asm volatile(
        "std\n"
        "rep movsl\n"
        "cld"
        : "+D"(dst), "+S"(src), "+c"(count)
        :
        : "memory"
      );
    }

    FillRectangle(
      Geometry2D::Rectangle(0, 0, width, rows),
      fillColor
    );
  }
}
