/**
 * @file Fonts/QBF.cpp
 * @brief Implements @ref @QFont::QBF.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "QBF.hpp"

namespace Quantum::Fonts {
  BitmapFont QBFParser::Parse(const UInt8* data, Size size) {
    BitmapFont font = {};

    if (!data || size < sizeof(QBFHeader)) return font;

    const QBFHeader* header = reinterpret_cast<const QBFHeader*>(data);

    if (header->Magic != QBFMagic) return font;
    if (header->Version < 1 || header->Version > 4) return font;
    if (header->Width == 0 || header->Height == 0) return font;
    if (header->Width > 32 || header->Height > 64) return font;
    if (header->GlyphCount == 0) return font;
    if (header->BytesPerGlyph == 0) return font;

    UInt32 glyphDataSize = static_cast<UInt32>(header->GlyphCount)
                         * header->BytesPerGlyph;
    UInt32 glyphDataEnd = header->GlyphDataOffset + glyphDataSize;

    if (glyphDataEnd < header->GlyphDataOffset) return font;
    if (glyphDataEnd > static_cast<UInt32>(size)) return font;

    font.GlyphData = data + header->GlyphDataOffset;
    font.Width = header->Width;
    font.Height = header->Height;
    font.BytesPerGlyph = header->BytesPerGlyph;
    font.GlyphCount = header->GlyphCount;
    font.Ascent = header->Ascent;
    font.BitsPerPixel = 1;

    // version 2: read extended header for bits-per-pixel
    if (header->Version >= 2) {
      Size v2ExtOffset = sizeof(QBFHeader);

      if (v2ExtOffset + sizeof(QBFHeaderV2Ext) <= size) {
        const auto* ext = reinterpret_cast<const QBFHeaderV2Ext*>(
          data + v2ExtOffset
        );

        if (ext->BitsPerPixel == 1
          || ext->BitsPerPixel == 8
          || ext->BitsPerPixel == 24
        ) {
          font.BitsPerPixel = ext->BitsPerPixel;
        }
      }
    }

    // version 3: read cap-height metrics for centering
    bool hasCapMetrics = false;

    if (header->Version >= 3) {
      Size v3ExtOffset = sizeof(QBFHeader) + sizeof(QBFHeaderV2Ext);

      if (v3ExtOffset + sizeof(QBFHeaderV3Ext) <= size) {
        const auto* ext = reinterpret_cast<const QBFHeaderV3Ext*>(
          data + v3ExtOffset
        );

        font.InkTop = ext->CapTop;
        font.InkHeight = ext->CapHeight;
        hasCapMetrics = true;
      }
    }

    // version 4: read subpixel order for LCD fonts
    if (header->Version >= 4) {
      Size v4ExtOffset = sizeof(QBFHeader) + sizeof(QBFHeaderV2Ext)
        + sizeof(QBFHeaderV3Ext);

      if (v4ExtOffset + sizeof(QBFHeaderV4Ext) <= size) {
        const auto* ext = reinterpret_cast<const QBFHeaderV4Ext*>(
          data + v4ExtOffset
        );

        font.SubpixelOrder = ext->SubpixelOrder;
      }
    }

    font.Advances = nullptr;

    if (header->AdvanceTableOffset != 0) {
      UInt32 advanceTableEnd = header->AdvanceTableOffset + header->GlyphCount;

      if (advanceTableEnd < header->AdvanceTableOffset) {
        font = {};

        return font;
      }

      if (advanceTableEnd > static_cast<UInt32>(size)) {
        font = {};

        return font;
      }

      font.Advances = data + header->AdvanceTableOffset;
    }

    if (!hasCapMetrics) {
      font.ComputeInkBounds();
    }

    return font;
  }
}
