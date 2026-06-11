/**
 * @file Core/Color.cpp
 * @brief Implements @ref @QCore::Color.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/Color.hpp>

namespace Quantum::Core {
  UInt32 Color::BlendARGB(
    UInt32 foreground,
    UInt32 background,
    UInt8 alpha
  ) {
    if (alpha == 255) return foreground;
    if (alpha == 0) return background;

    UInt32 inverseAlpha = 255 - alpha;
    UInt32 redBlue = (
      (foreground & 0xFF00FF) * alpha
      + (background & 0xFF00FF) * inverseAlpha
    ) >> 8;
    UInt32 green = (
      (foreground & 0x00FF00) * alpha
      + (background & 0x00FF00) * inverseAlpha
    ) >> 8;

    return 0xFF000000 | (redBlue & 0xFF00FF) | (green & 0x00FF00);
  }

  UInt16 Color::BlendRGB565(
    UInt16 foreground,
    UInt16 background,
    UInt8 alpha
  ) {
    if (alpha == 255) return foreground;
    if (alpha == 0) return background;

    UInt32 inverseAlpha = 255 - alpha;
    UInt32 foregroundRed = (foreground >> 11) & 0x1F;
    UInt32 backgroundRed = (background >> 11) & 0x1F;
    UInt32 foregroundGreen = (foreground >> 5) & 0x3F;
    UInt32 backgroundGreen = (background >> 5) & 0x3F;
    UInt32 foregroundBlue = foreground & 0x1F;
    UInt32 backgroundBlue = background & 0x1F;

    UInt32 red = (
      foregroundRed * alpha + backgroundRed * inverseAlpha
    ) / 255;
    UInt32 green = (
      foregroundGreen * alpha + backgroundGreen * inverseAlpha
    ) / 255;
    UInt32 blue = (
      foregroundBlue * alpha + backgroundBlue * inverseAlpha
    ) / 255;

    return static_cast<UInt16>((red << 11) | (green << 5) | blue);
  }

  UInt32 Color::BlendOver(
    UInt32 foreground,
    UInt32 background,
    UInt8 glyphAlpha
  ) {
    if (glyphAlpha == 255) return foreground | 0xFF000000;
    if (glyphAlpha == 0) return background;

    UInt8 backgroundAlpha = static_cast<UInt8>(
      (background >> 24) & 0xFF
    );

    UInt16 inverseGlyphAlpha = static_cast<UInt16>(255 - glyphAlpha);
    UInt16 outputAlpha = static_cast<UInt16>(
      glyphAlpha + (backgroundAlpha * inverseGlyphAlpha) / 255
    );

    if (outputAlpha == 0) return 0;

    UInt32 scaledBgAlpha = (backgroundAlpha * inverseGlyphAlpha) / 255;

    UInt8 red = static_cast<UInt8>(
      (((foreground >> 16) & 0xFF) * glyphAlpha
      + ((background >> 16) & 0xFF) * scaledBgAlpha)
      / outputAlpha
    );
    UInt8 green = static_cast<UInt8>(
      (((foreground >> 8) & 0xFF) * glyphAlpha
      + ((background >> 8) & 0xFF) * scaledBgAlpha)
      / outputAlpha
    );
    UInt8 blue = static_cast<UInt8>(
      ((foreground & 0xFF) * glyphAlpha
      + (background & 0xFF) * scaledBgAlpha)
      / outputAlpha
    );

    return (static_cast<UInt32>(outputAlpha) << 24)
      | (static_cast<UInt32>(red) << 16)
      | (static_cast<UInt32>(green) << 8)
      | blue;
  }

  UInt32 Color::BlendGammaARGB(
    UInt32 foreground,
    UInt32 background,
    UInt8 alpha
  ) {
    if (alpha == 255) return foreground;
    if (alpha == 0) return background;

    UInt32 inverseAlpha = 255 - alpha;

    UInt32 fgR = (foreground >> 16) & 0xFF;
    UInt32 bgR = (background >> 16) & 0xFF;
    UInt32 linR = (fgR * fgR * alpha
      + bgR * bgR * inverseAlpha) / 255;

    UInt32 fgG = (foreground >> 8) & 0xFF;
    UInt32 bgG = (background >> 8) & 0xFF;
    UInt32 linG = (fgG * fgG * alpha
      + bgG * bgG * inverseAlpha) / 255;

    UInt32 fgB = foreground & 0xFF;
    UInt32 bgB = background & 0xFF;
    UInt32 linB = (fgB * fgB * alpha
      + bgB * bgB * inverseAlpha) / 255;

    return 0xFF000000
      | (static_cast<UInt32>(_delinearize(linR)) << 16)
      | (static_cast<UInt32>(_delinearize(linG)) << 8)
      | _delinearize(linB);
  }

  UInt32 Color::BlendGammaOver(
    UInt32 foreground,
    UInt32 background,
    UInt8 glyphAlpha
  ) {
    if (glyphAlpha == 255) return foreground | 0xFF000000;
    if (glyphAlpha == 0) return background;

    UInt8 backgroundAlpha = static_cast<UInt8>(
      (background >> 24) & 0xFF
    );

    UInt16 inverseGlyphAlpha = static_cast<UInt16>(
      255 - glyphAlpha
    );
    UInt16 outputAlpha = static_cast<UInt16>(
      glyphAlpha + (backgroundAlpha * inverseGlyphAlpha) / 255
    );

    if (outputAlpha == 0) return 0;

    UInt32 scaledBgAlpha
      = (backgroundAlpha * inverseGlyphAlpha) / 255;

    UInt32 fgR = (foreground >> 16) & 0xFF;
    UInt32 bgR = (background >> 16) & 0xFF;
    UInt8 outR = _delinearize(
      (fgR * fgR * glyphAlpha + bgR * bgR * scaledBgAlpha)
      / outputAlpha
    );

    UInt32 fgG = (foreground >> 8) & 0xFF;
    UInt32 bgG = (background >> 8) & 0xFF;
    UInt8 outG = _delinearize(
      (fgG * fgG * glyphAlpha + bgG * bgG * scaledBgAlpha)
      / outputAlpha
    );

    UInt32 fgB = foreground & 0xFF;
    UInt32 bgB = background & 0xFF;
    UInt8 outB = _delinearize(
      (fgB * fgB * glyphAlpha + bgB * bgB * scaledBgAlpha)
      / outputAlpha
    );

    return (static_cast<UInt32>(outputAlpha) << 24)
      | (static_cast<UInt32>(outR) << 16)
      | (static_cast<UInt32>(outG) << 8)
      | outB;
  }

  UInt32 Color::BlendSubpixelARGB(
    UInt32 foreground,
    UInt32 background,
    UInt8 coverageR,
    UInt8 coverageG,
    UInt8 coverageB
  ) {
    UInt32 fgR = (foreground >> 16) & 0xFF;
    UInt32 bgR = (background >> 16) & 0xFF;
    UInt32 linR = (fgR * fgR * coverageR
      + bgR * bgR * (255 - coverageR)) / 255;

    UInt32 fgG = (foreground >> 8) & 0xFF;
    UInt32 bgG = (background >> 8) & 0xFF;
    UInt32 linG = (fgG * fgG * coverageG
      + bgG * bgG * (255 - coverageG)) / 255;

    UInt32 fgB = foreground & 0xFF;
    UInt32 bgB = background & 0xFF;
    UInt32 linB = (fgB * fgB * coverageB
      + bgB * bgB * (255 - coverageB)) / 255;

    return 0xFF000000
      | (static_cast<UInt32>(_delinearize(linR)) << 16)
      | (static_cast<UInt32>(_delinearize(linG)) << 8)
      | _delinearize(linB);
  }
}
