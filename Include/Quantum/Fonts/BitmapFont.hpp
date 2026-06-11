/**
 * @file Include/Quantum/Fonts/BitmapFont.hpp
 * @brief Declares @ref @QFont::BitmapFont.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "SubpixelOrder.hpp"

namespace Quantum::Fonts {
  /**
   * @brief Represents a bitmap font.
   */
  struct BitmapFont {
    /**
     * @brief Pointer to the first byte of glyph `0`, row `0`.
     */
    const UInt8* GlyphData;

    /**
     * @brief Cell width in pixels.
     */
    UInt8 Width;

    /**
     * @brief Cell height in pixels (rows per glyph).
     */
    UInt8 Height;

    /**
     * @brief Number of bytes per glyph (`Height * ceil(Width / 8)`).
     */
    UInt16 BytesPerGlyph;

    /**
     * @brief Total number of glyphs in the font.
     */
    UInt16 GlyphCount;

    /**
     * @brief Per-glyph horizontal advance widths, or `nullptr` for
     *        monospaced fonts.
     *
     * When non-null, points to an array of @ref GlyphCount bytes.
     * Each entry is the horizontal distance to advance after
     * rendering the corresponding glyph.

     */
    const UInt8* Advances;

    /**
     * @brief Baseline offset from the top of the cell in pixels.
     *
     * `0` means legacy top-aligned rendering (no baseline adjustment).
     */
    UInt8 Ascent;

    /**
     * @brief The glyph data's bits per pixel.
     */
    UInt8 BitsPerPixel = 1;

    /**
     * @brief Row index of the first row containing ink across all glyphs.
     *
     * Represents internal leading (empty rows above the tallest glyph).
     * `0` means the first row has ink.
     */
    UInt8 InkTop = 0;

    /**
     * @brief Number of rows from the first inked row to the last inked row
     *        (inclusive), measured across all printable ASCII glyphs.
     *
     * `0` means not computed; falls back to @ref Height.
     */
    UInt8 InkHeight = 0;

    /**
     * @brief Subpixel layout order, or @ref SubpixelOrder::None for
     *        grayscale / monochrome fonts.
     */
    SubpixelOrder SubpixelOrder = SubpixelOrder::None;

    /**
     * @brief Scans printable ASCII glyphs (33-126) to compute @ref InkTop
     *        and @ref InkHeight from the actual glyph data. Call once after
     *        populating all other fields.
     */
    void ComputeInkBounds() {
      UInt16 scanWidth;

      if (BitsPerPixel == 24) {
        scanWidth = static_cast<UInt16>(Width * 3);
      } else if (BitsPerPixel == 8) {
        scanWidth = Width;
      } else {
        scanWidth = static_cast<UInt16>((Width + 7) / 8);
      }

      // for multi-bit fonts, ignore faint AA fringe pixels that are
      // visually invisible but would inflate the ink extent
      UInt8 threshold = (BitsPerPixel >= 8) ? 80 : 0;

      UInt8 minInkRow = Height;
      UInt8 maxInkRow = 0;

      // scan uppercase letters and digits only to measure cap-height, which is
      // the visually relevant extent for UI centering
      // descenders and tall punctuation inflate the extent and push centered
      // text downward
      for (UInt16 enc = 48; enc <= 90 && enc < GlyphCount; ++enc) {
        if (enc > 57 && enc < 65) continue;

        const UInt8* glyph
          = GlyphData
          + static_cast<UInt32>(enc) * BytesPerGlyph;

        for (UInt8 row = 0; row < Height; ++row) {
          bool hasInk = false;

          for (UInt16 byteIndex = 0; byteIndex < scanWidth; ++byteIndex) {
            if (glyph[row * scanWidth + byteIndex] > threshold) {
              hasInk = true;

              break;
            }
          }

          if (hasInk) {
            if (row < minInkRow) minInkRow = row;
            if (row >= maxInkRow) maxInkRow = static_cast<UInt8>(row + 1);
          }
        }
      }

      if (maxInkRow > minInkRow) {
        InkTop = minInkRow;
        InkHeight = static_cast<UInt8>(maxInkRow - minInkRow);
      }
    }

    /**
     * @brief Returns a pointer to the row data for the given glyph.
     * @param index Glyph index (`0`-based).
     * @return Pointer to the first row byte, or `nullptr` if out of range.
     */
    constexpr const UInt8* GetGlyph(UInt16 index) const {
      if (index >= GlyphCount) return nullptr;

      return GlyphData + static_cast<UInt32>(index) * BytesPerGlyph;
    }

    /**
     * @brief Returns the number of bytes per row (`ceil(Width / 8)`).
     * @return The number of bytes per row for this font's glyph data.
     */
    constexpr UInt8 BytesPerRow() const {
      return static_cast<UInt8>((Width + 7) / 8);
    }

    /**
     * @brief Returns one row byte of a glyph (MSB = leftmost pixel).
     * @param index Glyph index (`0`-based).
     * @param row Row index within the glyph.
     * @return The row byte. Returns `0` if out of range.
     *
     * Only meaningful for fonts where `Width <= 8` (one byte per row).
     * This covers PSF v1 console fonts and the built-in Font8x14.
     */
    constexpr UInt8 GetRow(UInt16 index, UInt8 row) const {
      if (index >= GlyphCount || row >= Height) return 0;

      return GlyphData[static_cast<UInt32>(index) * BytesPerGlyph + row];
    }

    /**
     * @brief Tests whether a pixel is set in a glyph.
     * @param index Glyph index.
     * @param row Row index within the glyph.
     * @param column Column index within the glyph (`0` = leftmost).
     * @return `true` if the pixel is set, `false` otherwise or if
     *         any index is out of range.
     *
     * Works for any glyph width, including fonts wider than 8 pixels
     * where each row spans multiple bytes.
     */
    constexpr bool GetPixel(UInt16 index, UInt8 row, UInt8 column) const {
      if (index >= GlyphCount || row >= Height || column >= Width) {
        return false;
      }

      UInt8 bpr = BytesPerRow();
      UInt32 offset
        = static_cast<UInt32>(index) * BytesPerGlyph
        + static_cast<UInt32>(row) * bpr
        + (column / 8);
      UInt8 bit = static_cast<UInt8>(7 - (column % 8));

      return (GlyphData[offset] >> bit) & 1;
    }

    /**
     * @brief Returns the alpha (coverage) value for a glyph pixel.
     * @param index Glyph index.
     * @param row Row index within the glyph.
     * @param column Column index within the glyph (`0` = leftmost).
     * @return 0-255 coverage value (0 = transparent, 255 = fully opaque).
     *         For 1bpp fonts returns 0 or 255. For 8bpp fonts returns the
     *         stored grayscale value directly. For 24bpp LCD fonts returns
     *         the green channel as a representative coverage.
     */
    constexpr UInt8 GetAlpha(UInt16 index, UInt8 row, UInt8 column) const {
      if (index >= GlyphCount || row >= Height || column >= Width) return 0;

      if (BitsPerPixel == 24) {
        UInt32 offset
          = static_cast<UInt32>(index) * BytesPerGlyph
          + (static_cast<UInt32>(row) * Width + column) * 3
          + 1;

        return GlyphData[offset];
      }

      if (BitsPerPixel == 8) {
        UInt32 offset
          = static_cast<UInt32>(index) * BytesPerGlyph
          + static_cast<UInt32>(row) * Width
          + column;

        return GlyphData[offset];
      }

      return GetPixel(index, row, column) ? 255 : 0;
    }

    /**
     * @brief Returns per-channel LCD coverage for a glyph pixel.
     * @param index Glyph index.
     * @param row Row index within the glyph.
     * @param column Column index within the glyph (`0` = leftmost).
     * @param coverageR Output red subpixel coverage (0-255).
     * @param coverageG Output green subpixel coverage (0-255).
     * @param coverageB Output blue subpixel coverage (0-255).
     *
     * Only meaningful for 24bpp LCD fonts. For 8bpp and 1bpp fonts,
     * all three channels are set to the same value as @ref GetAlpha.
     */
    constexpr void GetLCDCoverage(
      UInt16 index,
      UInt8 row,
      UInt8 column,
      UInt8& coverageR,
      UInt8& coverageG,
      UInt8& coverageB
    ) const {
      if (index >= GlyphCount || row >= Height || column >= Width) {
        coverageR = coverageG = coverageB = 0;

        return;
      }

      if (BitsPerPixel == 24) {
        UInt32 offset
          = static_cast<UInt32>(index) * BytesPerGlyph
          + (static_cast<UInt32>(row) * Width + column) * 3;

        coverageR = GlyphData[offset];
        coverageG = GlyphData[offset + 1];
        coverageB = GlyphData[offset + 2];

        return;
      }

      UInt8 alpha = GetAlpha(index, row, column);

      coverageR = coverageG = coverageB = alpha;
    }

    /**
     * @brief Returns the horizontal advance width for a glyph.
     * @param index Glyph index.
     * @return The advance width in pixels. Returns @ref Width if
     *         the font is monospaced or the index is out of range.
     */
    constexpr UInt8 GetAdvance(UInt16 index) const {
      if (!Advances || index >= GlyphCount) return Width;

      return Advances[index];
    }

    /**
     * @brief Returns whether this font has per-glyph advance widths.
     * @return `true` if this font is proportional (has a non-null
     *         @ref Advances pointer), or `false` if monospaced.
     */
    constexpr bool IsProportional() const {
      return Advances != nullptr;
    }

    /**
     * @brief Returns whether this font uses LCD subpixel coverage.
     * @return `true` if this font is an LCD font with 24bpp glyph data,
     *         or `false` otherwise.
     */
    constexpr bool IsLCD() const {
      return BitsPerPixel == 24;
    }

    /**
     * @brief Computes the total pixel width of a null-terminated string.
     * @param text The string to measure.
     * @return Total width in pixels.
     */
    constexpr UInt16 TextWidth(const char* text) const {
      if (!text) return 0;

      UInt16 w = 0;

      for (Size i = 0; text[i]; ++i) {
        w = static_cast<UInt16>(
          w + GetAdvance(static_cast<UInt8>(text[i]))
        );
      }

      return w;
    }

    /**
     * @brief Computes the Y offset to vertically center text within a
     *        container of the given height.
     * @param containerHeight The height of the container in pixels.
     * @return The Y offset from the container's top edge to pass as the
     *         text drawing Y coordinate (top of the glyph cell).
     *
     * Centers the cap-height region (from @ref InkTop to the baseline
     * at @ref Ascent) within the container. This ignores descenders
     * so that UI text appears optically centered.
     */
    constexpr Int16 CenterTextY(UInt16 containerHeight) const {
      UInt16 visualHeight = (InkHeight > 0) ? InkHeight : Height;

      // round up so text sits slightly low rather than high when the
      // remainder is odd - visually text looks better grounded
      Int16 centered = static_cast<Int16>(
        (containerHeight - visualHeight + 1) / 2
      );

      return static_cast<Int16>(centered - InkTop);
    }

    /**
     * @brief Computes the total pixel width of a counted string.
     * @param text The string to measure.
     * @param count Maximum number of characters to measure.
     * @return Total width in pixels.
     */
    constexpr UInt16 TextWidth(const char* text, Size count) const {
      if (!text) return 0;

      UInt16 w = 0;

      for (Size i = 0; i < count && text[i]; ++i) {
        w = static_cast<UInt16>(
          w + GetAdvance(static_cast<UInt8>(text[i]))
        );
      }

      return w;
    }
  };
}
