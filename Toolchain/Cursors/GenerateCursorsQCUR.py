#!/usr/bin/env python3
"""
Generates a QCUR (Quantum Cursor) file containing the system cursor set.

Usage:
  python3 GenerateCursorsQCUR.py <output.qcur>

Each cursor is a 16x16 grid of pixels defined using character codes:
  K = black (0xFF000000)
  W = white (0xFFFFFFFF)
  . = transparent (0x00000000)
"""

import struct
import sys

QCUR_MAGIC = 0x51435231  # "QCR1"
QCUR_VERSION = 1
WIDTH = 16
HEIGHT = 16

K = 0xFF000000
W = 0xFFFFFFFF
T = 0x00000000

COLOR_MAP = {"K": K, "W": W, ".": T}


def parse_grid(lines, hotspot=(0, 0)):
    """Parse a 16x16 character grid into pixel data + hotspot."""
    pixels = []

    for row in lines:
        # pad or truncate to WIDTH
        row = row.ljust(WIDTH, ".")[:WIDTH]

        for ch in row:
            pixels.append(COLOR_MAP.get(ch, T))

    assert len(pixels) == WIDTH * HEIGHT, f"Expected {WIDTH * HEIGHT} pixels, got {len(pixels)}"

    return pixels, hotspot


# ----------------------------------------------------------------
# Cursor definitions (Mac OS 9-inspired, original pixel art)
# ----------------------------------------------------------------

# 0: Arrow - directly from Cursor.hpp (white outline, black fill)
ARROW = parse_grid([
    "WW..............",
    "WKW.............",
    "WKKW............",
    "WKKKW...........",
    "WKKKKW..........",
    "WKKKKKW.........",
    "WKKKKKKW........",
    "WKKKKKKKW.......",
    "WKKKKKKKKW......",
    "WKKKKKWWWW......",
    "WKKWKKW.........",
    "WKW.WKKW........",
    "WW..WKKW........",
    ".....WKKW.......",
    ".....WKKW.......",
    "......WW........",
], hotspot=(0, 0))

# 1: IBeam - text selection cursor
IBEAM = parse_grid([
    "..WKW.WKW.......",
    "...WWKWW........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "....WKW.........",
    "...WWKWW........",
    "..WKW.WKW.......",
    "................",
], hotspot=(4, 7))

# 2: Crosshair - precision select
CROSSHAIR = parse_grid([
    "......WKW.......",
    "......WKW.......",
    "......WKW.......",
    "......WKW.......",
    "......WKW.......",
    "......W.W.......",
    "WWWWWW...WWWWWW.",
    "KKKKKK.K.KKKKKK",
    "WWWWWW...WWWWWW.",
    "......W.W.......",
    "......WKW.......",
    "......WKW.......",
    "......WKW.......",
    "......WKW.......",
    "......WKW.......",
    "................",
], hotspot=(7, 7))

# 3: Watch - busy/wait (wristwatch)
WATCH = parse_grid([
    "..WWWWWWWWWW....",
    "..WKKKKKKKKW....",
    ".WKWWWWWWWWKW...",
    ".WKWKKKKKKWKW...",
    ".WKWK....KWKW...",
    ".WKWK.K..KWKW...",
    ".WKWK..K.KWKW...",
    ".WKWK...KKWKW...",
    ".WKWK....KWKW...",
    ".WKWK....KWKW...",
    ".WKWKKKKKKWKW...",
    ".WKWWWWWWWWKW...",
    "..WKKKKKKKKW....",
    "..WWWWWWWWWW....",
    "................",
    "................",
], hotspot=(7, 7))

# 4: Hand - pointing hand / link cursor
HAND = parse_grid([
    "......WW........",
    ".....WKKW.......",
    ".....WKKW.......",
    ".....WKKW.......",
    ".WW..WKKWWW.....",
    "WKKWWWKKWKKWW...",
    "WKKWKKKKKKWKKW..",
    ".WKKKKKKKKKKWW..",
    "..WKKKKKKKKKKW..",
    "..WKKKKKKKKKKW..",
    "...WKKKKKKKKW...",
    "...WKKKKKKKKW...",
    "....WKKKKKKKW...",
    "....WKKKKKKKW...",
    ".....WWWWWWWW...",
    "................",
], hotspot=(5, 0))

# 5: ResizeNWSE - directly from ResizeCursor.hpp
RESIZE_NWSE = parse_grid([
    "WWWW............",
    "WKKW............",
    "WKKW............",
    "WWWKW...........",
    "...WKW..........",
    "....WKW.........",
    ".....WKW........",
    "......WKW.......",
    ".......WKW......",
    "........WKW.....",
    ".........WKWWW..",
    "..........WKKW..",
    "..........WKKW..",
    "..........WWWW..",
    "................",
    "................",
], hotspot=(7, 7))

# 6: ResizeNESW - NE-SW diagonal resize (mirror of NWSE)
RESIZE_NESW = parse_grid([
    "..........WWWW..",
    "..........WKKW..",
    "..........WKKW..",
    ".........WKW.W..",
    "........WKW.....",
    ".......WKW......",
    "......WKW.......",
    ".....WKW........",
    "....WKW.........",
    "...WKW..........",
    "..WWKW..........",
    "..WKKW..........",
    "..WKKW..........",
    "..WWWW..........",
    "................",
    "................",
], hotspot=(7, 7))

# 7: ResizeNS - vertical resize
RESIZE_NS = parse_grid([
    ".....WKW........",
    "....WKKKW.......",
    "...WKKKKKW......",
    "..WWWKKKWWW.....",
    "....WKKKW.......",
    "....WKKKW.......",
    "....WKKKW.......",
    "....WKKKW.......",
    "....WKKKW.......",
    "....WKKKW.......",
    "....WKKKW.......",
    "..WWWKKKWWW.....",
    "...WKKKKKW......",
    "....WKKKW.......",
    ".....WKW........",
    "................",
], hotspot=(5, 7))

# 8: ResizeEW - horizontal resize
RESIZE_EW = parse_grid([
    "................",
    "................",
    "....W.....W.....",
    "...WK.....KW....",
    "..WKK.....KKW...",
    ".WKKKKKKKKKKKW..",
    ".WKKKKKKKKKKKW..",
    ".WKKKKKKKKKKKW..",
    "..WKK.....KKW...",
    "...WK.....KW....",
    "....W.....W.....",
    "................",
    "................",
    "................",
    "................",
    "................",
], hotspot=(7, 5))

CURSORS = [
    ARROW,
    IBEAM,
    CROSSHAIR,
    WATCH,
    HAND,
    RESIZE_NWSE,
    RESIZE_NESW,
    RESIZE_NS,
    RESIZE_EW,
]


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <output.qcur>", file=sys.stderr)
        sys.exit(1)

    output_path = sys.argv[1]
    cursor_count = len(CURSORS)

    # layout: header (16 bytes) + hotspot table + pixel data
    header_size = 16
    hotspot_table_size = cursor_count * 2  # X bytes then Y bytes
    hotspot_table_offset = header_size
    pixel_data_offset = header_size + hotspot_table_size
    pixels_per_cursor = WIDTH * HEIGHT
    bytes_per_cursor = pixels_per_cursor * 4

    header = struct.pack(
        "<IBBBBII",
        QCUR_MAGIC,
        QCUR_VERSION,
        WIDTH,
        HEIGHT,
        cursor_count,
        pixel_data_offset,
        hotspot_table_offset,
    )

    # build hotspot table
    hotspot_x = bytes(c[1][0] for c in CURSORS)
    hotspot_y = bytes(c[1][1] for c in CURSORS)
    hotspot_table = hotspot_x + hotspot_y

    # build pixel data
    pixel_data = bytearray()

    for pixels, _ in CURSORS:
        for pixel in pixels:
            pixel_data.extend(struct.pack("<I", pixel))

    with open(output_path, "wb") as f:
        f.write(header)
        f.write(hotspot_table)
        f.write(pixel_data)

    total = len(header) + len(hotspot_table) + len(pixel_data)

    print(f"  Wrote {output_path} ({total} bytes)")
    print(f"  {cursor_count} cursors, {WIDTH}x{HEIGHT}, 32-bit ARGB")
    print("Done.")


if __name__ == "__main__":
    main()
