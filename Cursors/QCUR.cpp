/**
 * @file Cursors/QCUR.cpp
 * @brief Implements @ref @QCursors::QCUR.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "QCUR.hpp"

namespace Quantum::Cursors {
  CursorImage QCURParser::Parse(const UInt8* data, Size size) {
    CursorImage cursor = {};

    if (!data || size < sizeof(QCURHeader)) return cursor;

    const QCURHeader* header = reinterpret_cast<const QCURHeader*>(data);

    if (
      header->Magic != QCURMagic ||
      header->Version != 1 ||
      header->Width == 0 ||
      header->Height == 0 ||
      header->Width > 64 ||
      header->Height > 64 ||
      header->CursorCount == 0
    ) return cursor;

    UInt32 pixelsPerCursor
      = static_cast<UInt32>(header->Width)
      * header->Height;
    UInt32 bytesPerCursor = pixelsPerCursor * 4;
    UInt32 totalPixelBytes
      = static_cast<UInt32>(header->CursorCount)
      * bytesPerCursor;
    UInt32 pixelDataEnd = header->PixelDataOffset + totalPixelBytes;

    if (
      pixelDataEnd < header->PixelDataOffset ||
      pixelDataEnd > static_cast<UInt32>(size)
    ) return cursor;

    cursor.PixelData = reinterpret_cast<const UInt32*>(
      data + header->PixelDataOffset
    );
    cursor.Width = header->Width;
    cursor.Height = header->Height;
    cursor.CursorCount = header->CursorCount;

    if (header->HotspotTableOffset != 0) {
      UInt32 hotspotTableSize = static_cast<UInt32>(header->CursorCount) * 2;
      UInt32 hotspotEnd = header->HotspotTableOffset + hotspotTableSize;

      if (
        hotspotEnd > header->HotspotTableOffset &&
        hotspotEnd <= static_cast<UInt32>(size)
      ) {
        cursor.HotspotX = data + header->HotspotTableOffset;
        cursor.HotspotY
          = data
          + header->HotspotTableOffset
          + header->CursorCount;
      }
    }

    return cursor;
  }
}
