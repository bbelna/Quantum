/**
 * @file Include/Quantum/Fonts/PSF.hpp
 * @brief Declares @ref @QFont::PSF and related structures.
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
   * @brief PSF v1 file magic number (`0x0436`).
   */
  static constexpr UInt16 PSF1Magic = 0x0436;

  /**
   * @brief PSF v2 file magic number (`0x864AB572`).
   */
  static constexpr UInt32 PSF2Magic = 0x864AB572;

  /**
   * @brief PSF v1 header mode flags.
   */
  enum class PSF1Mode : UInt8 {
    /**
     * @brief No flags set (256 glyphs, no Unicode table).
     */
    None = 0,

    /**
     * @brief The font contains 512 glyphs instead of 256.
     */
    Mode512 = 1,

    /**
     * @brief A Unicode mapping table follows the glyph data.
     */
    HasUnicodeTable = 2,

    /**
     * @brief The Unicode table contains sequences.
     */
    HasSequences = 4,
  };

  /**
   * @brief Raw PSF v1 file header (4 bytes).
   */
  struct PSF1Header {
    /**
     * @brief Magic number (must be @ref PSF1Magic).
     */
    UInt16 Magic;

    /**
     * @brief Mode flags (see @ref PSF1Mode).
     */
    UInt8 Mode;

    /**
     * @brief Bytes per glyph (equals the glyph height for 8px-wide fonts).
     */
    UInt8 CharSize;
  };

  /**
   * @brief PSF v2 header flags.
   */
  enum class PSF2Flags : UInt32 {
    /**
     * @brief No flags set.
     */
    None = 0,

    /**
     * @brief The font file contains a Unicode mapping table after
     *        the glyph data.
     */
    HasUnicodeTable = 1,
  };

  /**
   * @brief Raw PSF v2 file header (32 bytes, little-endian).
   */
  struct PSF2Header {
    /**
     * @brief Magic number (must be @ref PSF2Magic).
     */
    UInt32 Magic;

    /**
     * @brief Format version (must be `0`).
     */
    UInt32 Version;

    /**
     * @brief Offset to the start of glyph data (`>= 32`).
     */
    UInt32 HeaderSize;

    /**
     * @brief Flags (see @ref PSF2Flags).
     */
    UInt32 Flags;

    /**
     * @brief Number of glyphs in the font.
     */
    UInt32 GlyphCount;

    /**
     * @brief Number of bytes per glyph.
     */
    UInt32 GlyphSizeInBytes;

    /**
     * @brief Glyph height in pixels.
     */
    UInt32 Height;

    /**
     * @brief Glyph width in pixels.
     */
    UInt32 Width;
  };

  /**
   * @brief Parser for the PC Screen Font (PSF) format, versions 1 and 2.
   *
   * The returned @ref BitmapFont points into the provided buffer
   * (non-owning). The caller must ensure the buffer remains valid
   * for the lifetime of the font.
   *
   * On failure, the returned font has `GlyphData == nullptr`.
   *
   * This class performs no allocation and no file I/O.
   */
  class PSFParser {
    public:
      /**
       * @brief Parses a PSF font (v1 or v2) from a raw byte buffer,
       *        auto-detecting the version from the magic bytes.
       * @param data Pointer to the start of the PSF file data.
       * @param size Size of the data in bytes.
       * @return Parsed font, or a zeroed font with `GlyphData == nullptr`
       *         on failure.
       */
      static BitmapFont Parse(const UInt8* data, Size size);

      /**
       * @brief Parses a PSF v1 font from a raw byte buffer.
       * @param data Pointer to the start of the PSF v1 file data.
       * @param size Size of the data in bytes.
       * @return Parsed font, or a zeroed font with `GlyphData == nullptr`
       *         on failure.
       */
      static BitmapFont ParseV1(const UInt8* data, Size size);

      /**
       * @brief Parses a PSF v2 font from a raw byte buffer.
       * @param data Pointer to the start of the PSF v2 file data.
       * @param size Size of the data in bytes.
       * @return Parsed font, or a zeroed font with `GlyphData == nullptr`
       *         on failure.
       */
      static BitmapFont ParseV2(const UInt8* data, Size size);
  };
}
