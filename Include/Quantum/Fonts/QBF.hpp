/**
 * @file Include/Quantum/Fonts/QBF.hpp
 * @brief Declares @ref @QFont::QBF and related structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "BitmapFont.hpp"

namespace Quantum::Fonts {
  /**
   * @brief QBF file magic number (`0x51424631`, `"QBF1"` in ASCII).
   */
  static constexpr UInt32 QBFMagic = 0x51424631;

  /**
   * @brief Raw QBF file header (20 bytes, little-endian).
   */
  struct QBFHeader {
    /**
     * @brief Magic number (must be @ref QBFMagic).
     */
    UInt32 Magic;

    /**
     * @brief Format version (must be `1`).
     */
    UInt8 Version;

    /**
     * @brief Maximum cell width in pixels.
     */
    UInt8 Width;

    /**
     * @brief Cell height in pixels.
     */
    UInt8 Height;

    /**
     * @brief Baseline offset from the top of the cell.
     */
    UInt8 Ascent;

    /**
     * @brief Number of bytes per glyph. For 1bpp:
     *        `Height * ceil(Width / 8)`. For 8bpp: `Height * Width`.
     */
    UInt16 BytesPerGlyph;

    /**
     * @brief Number of glyphs in the font.
     */
    UInt16 GlyphCount;

    /**
     * @brief Byte offset from file start to glyph bitmap data.
     */
    UInt32 GlyphDataOffset;

    /**
     * @brief Byte offset from file start to the advance width table,
     *        or `0` if the font is monospaced.
     */
    UInt32 AdvanceTableOffset;
  };

  /**
   * @brief Extended header fields present when @ref QBFHeader::Version >= 2.
   *        Immediately follows the 20-byte @ref QBFHeader in the file.
   */
  struct QBFHeaderV2Ext {
    /**
     * @brief Bits per pixel: `1` for packed monochrome (MSB-first),
     *        `8` for grayscale (one byte per pixel, 0 = transparent,
     *        255 = fully opaque). Version 1 files implicitly have
     *        `BitsPerPixel = 1`.
     */
    UInt8 BitsPerPixel;
  };

  /**
   * @brief Extended header fields present when @ref QBFHeader::Version >= 3.
   *        Immediately follows the @ref QBFHeaderV2Ext in the file.
   */
  struct QBFHeaderV3Ext {
    /**
     * @brief Row of the first inked pixel in uppercase glyphs (A-Z).
     */
    UInt8 CapTop;

    /**
     * @brief Height in pixels from @ref CapTop to the baseline.
     *        Used for vertical centering of UI text.
     */
    UInt8 CapHeight;
  };

  /**
   * @brief Extended header fields present when @ref QBFHeader::Version >= 4.
   *        Immediately follows the @ref QBFHeaderV3Ext in the file.
   */
  struct QBFHeaderV4Ext {
    /**
     * @brief Subpixel layout order used during rasterization.
     *        Only meaningful when `BitsPerPixel == 24`.
     */
    SubpixelOrder SubpixelOrder;
  };

  /**
   * @brief Parser for the QBF (Quantum Bitmap Font) format.
   *
   * The returned @ref BitmapFont points into the provided buffer
   * (non-owning). The caller must ensure the buffer remains valid
   * for the lifetime of the font.
   *
   * On failure, the returned font has `GlyphData == nullptr`.
   *
   * This class performs no allocation and no file I/O.
   */
  class QBFParser {
    public:
      /**
       * @brief Parses a QBF font from a raw byte buffer.
       * @param data Pointer to the start of the QBF file data.
       * @param size Size of the data in bytes.
       * @return Parsed font, or a zeroed font with `GlyphData == nullptr`
       *         on failure.
       */
      static BitmapFont Parse(const UInt8* data, Size size);
  };
}
