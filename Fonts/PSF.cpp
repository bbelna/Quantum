/**
 * @file Fonts/PSF.cpp
 * @brief Implements @ref @QFont::PSF.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PSF.hpp"

namespace Quantum::Fonts {
  BitmapFont PSFParser::ParseV1(const UInt8* data, Size size) {
    BitmapFont font = {};

    if (!data || size < sizeof(PSF1Header)) return font;

    const PSF1Header* header = reinterpret_cast<const PSF1Header*>(data);

    if (header->Magic != PSF1Magic) return font;
    if (header->CharSize == 0) return font;

    UInt16 glyphCount = (header->Mode & static_cast<UInt8>(PSF1Mode::Mode512))
      ? 512
      : 256;

    UInt32 glyphDataSize = static_cast<UInt32>(glyphCount) * header->CharSize;
    UInt32 glyphDataEnd = sizeof(PSF1Header) + glyphDataSize;

    if (glyphDataEnd > static_cast<UInt32>(size)) return font;

    font.GlyphData = data + sizeof(PSF1Header);
    font.Width = 8;
    font.Height = header->CharSize;
    font.BytesPerGlyph = header->CharSize;
    font.GlyphCount = glyphCount;
    font.ComputeInkBounds();

    font.Ascent = (font.InkHeight > 0)
      ? static_cast<UInt8>(font.InkTop + font.InkHeight)
      : static_cast<UInt8>(header->CharSize > 4 ? header->CharSize - 4 : header->CharSize);

    return font;
  }

  BitmapFont PSFParser::ParseV2(const UInt8* data, Size size) {
    BitmapFont font = {};

    if (!data || size < sizeof(PSF2Header)) return font;

    const PSF2Header* header = reinterpret_cast<const PSF2Header*>(data);

    if (header->Magic != PSF2Magic) return font;
    if (header->HeaderSize < sizeof(PSF2Header)) return font;
    if (header->Width == 0 || header->Height == 0) return font;
    if (header->Width > 32 || header->Height > 64) return font;
    if (header->GlyphCount == 0) return font;
    if (header->GlyphSizeInBytes == 0) return font;

    UInt32 glyphDataSize = header->GlyphCount * header->GlyphSizeInBytes;
    UInt32 glyphDataEnd = header->HeaderSize + glyphDataSize;

    if (glyphDataEnd < header->HeaderSize) return font;
    if (glyphDataEnd > static_cast<UInt32>(size)) return font;

    font.GlyphData = data + header->HeaderSize;
    font.Width = static_cast<UInt8>(header->Width);
    font.Height = static_cast<UInt8>(header->Height);
    font.BytesPerGlyph = static_cast<UInt16>(header->GlyphSizeInBytes);
    font.GlyphCount = static_cast<UInt16>(header->GlyphCount);
    font.ComputeInkBounds();

    font.Ascent = (font.InkHeight > 0)
      ? static_cast<UInt8>(font.InkTop + font.InkHeight)
      : static_cast<UInt8>(
          header->Height > 4 ? header->Height - 4 : header->Height
        );

    return font;
  }

  BitmapFont PSFParser::Parse(const UInt8* data, Size size) {
    BitmapFont font = {};

    if (!data || size < 4) return font;

    // check PSF v2 first (4-byte magic)
    UInt32 magic32 = static_cast<UInt32>(data[0])
                   | (static_cast<UInt32>(data[1]) << 8)
                   | (static_cast<UInt32>(data[2]) << 16)
                   | (static_cast<UInt32>(data[3]) << 24);

    if (magic32 == PSF2Magic) return ParseV2(data, size);

    // check PSF v1 (2-byte magic)
    UInt16 magic16 = static_cast<UInt16>(
      data[0] | (static_cast<UInt16>(data[1]) << 8)
    );

    if (magic16 == PSF1Magic) return ParseV1(data, size);

    return font;
  }
}
