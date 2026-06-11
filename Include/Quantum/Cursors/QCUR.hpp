/**
 * @file Include/Quantum/Cursors/QCUR.hpp
 * @brief Declares @ref @QCursors::QCUR and related structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "CursorImage.hpp"

namespace Quantum::Cursors {
  /**
   * @brief QCUR file magic number (`0x51435231`, `"QCR1"` in ASCII).
   */
  static constexpr UInt32 QCURMagic = 0x51435231;

  /**
   * @brief Raw QCUR file header (16 bytes, little-endian).
   */
  struct __attribute__((packed)) QCURHeader {
    /**
     * @brief Magic number (must be @ref QCURMagic).
     */
    UInt32 Magic;

    /**
     * @brief Format version (must be `1`).
     */
    UInt8 Version;

    /**
     * @brief Cursor width in pixels.
     */
    UInt8 Width;

    /**
     * @brief Cursor height in pixels.
     */
    UInt8 Height;

    /**
     * @brief Number of cursors in the file.
     */
    UInt8 CursorCount;

    /**
     * @brief Byte offset from file start to the ARGB pixel data.
     */
    UInt32 PixelDataOffset;

    /**
     * @brief Byte offset from file start to the hotspot table, or
     *        `0` if all cursors have hotspot (0, 0). The table is
     *        `CursorCount` bytes of X values followed by
     *        `CursorCount` bytes of Y values.
     */
    UInt32 HotspotTableOffset;
  };

  /**
   * @brief Parser for the QCUR (Quantum Cursor) format.
   *
   * The returned @ref CursorImage points into the provided buffer
   * (non-owning). The caller must ensure the buffer remains valid
   * for the lifetime of the cursor set.
   *
   * On failure, the returned cursor set has `PixelData == nullptr`.
   *
   * This class performs no allocation and no file I/O.
   */
  class QCURParser {
    public:
      /**
       * @brief Parses a QCUR cursor set from a raw byte buffer.
       * @param data Pointer to the start of the QCUR file data.
       * @param size Size of the data in bytes.
       * @return Parsed cursor set, or a zeroed set with
       *         `PixelData == nullptr` on failure.
       */
      static CursorImage Parse(const UInt8* data, Size size);
  };
}
