#!/usr/bin/env python3
"""
ConvertBDFtoPSF2.py — Convert a BDF bitmap font to PSF v2 format.

Usage:
    python3 ConvertBDFtoPSF2.py <input.bdf> <output.psf> [--glyphs=256]

Produces a PSF v2 file with an optional Unicode mapping table.
Only codepoints 0..glyphs-1 are included; missing glyphs are blank.
"""

import argparse
import struct
import sys
from pathlib import Path

PSF2_MAGIC = 0x864AB572
PSF2_HAS_UNICODE_TABLE = 0x01
PSF2_SEPARATOR = b"\xFF"
PSF2_STARTSEQ = b"\xFE"


def parse_bdf(path):
    """Parse a BDF file and return (font_bbx, glyphs).

    font_bbx = (width, height, x_off, y_off)
    glyphs   = dict mapping encoding -> (bbx, [row_ints])
    """
    lines = Path(path).read_text().splitlines()
    font_bbx = None
    glyphs = {}

    i = 0
    while i < len(lines):
        parts = lines[i].split()

        if parts[0] == "FONTBOUNDINGBOX":
            font_bbx = (int(parts[1]), int(parts[2]),
                        int(parts[3]), int(parts[4]))

        elif parts[0] == "STARTCHAR":
            encoding = None
            char_bbx = None

            i += 1
            while i < len(lines) and lines[i].split()[0] != "BITMAP":
                p = lines[i].split()
                if p[0] == "ENCODING":
                    encoding = int(p[1])
                elif p[0] == "BBX":
                    char_bbx = (int(p[1]), int(p[2]),
                                int(p[3]), int(p[4]))
                i += 1

            # now at BITMAP line — collect hex rows
            i += 1
            rows = []
            while i < len(lines) and lines[i].strip() != "ENDCHAR":
                rows.append(int(lines[i].strip(), 16))
                i += 1

            if encoding is not None and encoding >= 0:
                glyphs[encoding] = (char_bbx or font_bbx, rows)

        i += 1

    return font_bbx, glyphs


def place_glyph(font_bbx, char_bbx, char_rows):
    """Place a glyph's bitmap into the font's bounding box, returning
    a list of `font_height` row integers, each `ceil(font_width/8)*8`
    bits wide (MSB = leftmost pixel)."""
    fw, fh, fx, fy = font_bbx
    cw, ch, cx, cy = char_bbx

    bytes_per_row = (fw + 7) // 8
    out = [0] * fh

    # BDF coordinates: baseline is at y=0, ascent goes up.
    # font_bbx y offset (fy) is the bottom of the box relative to baseline.
    # char_bbx y offset (cy) is the bottom of the char relative to baseline.
    # In pixel rows (top-down), row 0 = top of font bounding box.
    # The font ascent = fh + fy (since fy is typically negative).
    font_ascent = fh + fy

    # The char's top pixel row in font-box coordinates:
    char_top = font_ascent - (cy + ch)

    # Horizontal shift: char x-offset relative to font x-offset
    x_shift = cx - fx

    for r, row_val in enumerate(char_rows):
        dst_row = char_top + r
        if dst_row < 0 or dst_row >= fh:
            continue

        # row_val has cw significant bits, MSB-first, in the top bits
        # of however many hex nibbles were in the BDF.  We need to know
        # how many bits are in row_val.  BDF hex rows are always padded
        # to full bytes, so the number of bits is ceil(cw/8)*8.
        src_bits = ((cw + 7) // 8) * 8

        # extract each pixel and place it
        for col in range(cw):
            bit = (row_val >> (src_bits - 1 - col)) & 1
            if not bit:
                continue
            dst_col = x_shift + col
            if dst_col < 0 or dst_col >= fw:
                continue
            # set bit in output row (MSB = leftmost)
            byte_idx = dst_col // 8
            bit_idx = 7 - (dst_col % 8)
            out[dst_row] |= (1 << bit_idx) << (byte_idx * 0)
            # actually, we need to build the row as bytes
            # let's redo with byte array approach

    # redo with proper byte-array approach
    out_bytes = [bytearray(bytes_per_row) for _ in range(fh)]

    for r, row_val in enumerate(char_rows):
        dst_row = char_top + r
        if dst_row < 0 or dst_row >= fh:
            continue

        src_bits = ((cw + 7) // 8) * 8

        for col in range(cw):
            bit = (row_val >> (src_bits - 1 - col)) & 1
            if not bit:
                continue
            dst_col = x_shift + col
            if dst_col < 0 or dst_col >= fw:
                continue
            byte_idx = dst_col // 8
            bit_idx = 7 - (dst_col % 8)
            out_bytes[dst_row][byte_idx] |= (1 << bit_idx)

    return out_bytes


def build_psf2(font_bbx, bdf_glyphs, glyph_count):
    """Build a PSF v2 binary from parsed BDF data."""
    fw, fh, _, _ = font_bbx
    bytes_per_row = (fw + 7) // 8
    bytes_per_glyph = fh * bytes_per_row

    # build glyph data and unicode table
    glyph_data = bytearray()
    unicode_entries = []

    for g in range(glyph_count):
        if g in bdf_glyphs:
            char_bbx, char_rows = bdf_glyphs[g]
            rows = place_glyph(font_bbx, char_bbx, char_rows)
            for row in rows:
                glyph_data.extend(row)
        else:
            glyph_data.extend(b"\x00" * bytes_per_glyph)

        # unicode mapping: glyph g maps to codepoint g
        if g < 0x80:
            unicode_entries.append(bytes([g]) + PSF2_SEPARATOR)
        else:
            # encode as UTF-8
            unicode_entries.append(chr(g).encode("utf-8") + PSF2_SEPARATOR)

    # assemble unicode table
    unicode_table = b"".join(unicode_entries)

    # PSF v2 header
    header = struct.pack(
        "<IIIIIIII",
        PSF2_MAGIC,
        0,              # version
        32,             # header size
        PSF2_HAS_UNICODE_TABLE,
        glyph_count,
        bytes_per_glyph,
        fh,             # height
        fw,             # width
    )

    return header + bytes(glyph_data) + unicode_table


def main():
    parser = argparse.ArgumentParser(
        description="Convert a BDF bitmap font to PSF v2 format."
    )
    parser.add_argument("input", help="Input BDF file")
    parser.add_argument("output", help="Output PSF v2 file")
    parser.add_argument(
        "--glyphs", type=int, default=256,
        help="Number of glyphs to include (default: 256)"
    )

    args = parser.parse_args()

    font_bbx, bdf_glyphs = parse_bdf(args.input)

    if font_bbx is None:
        print("error: no FONTBOUNDINGBOX in BDF file", file=sys.stderr)
        sys.exit(1)

    print(
        f"  BDF: {font_bbx[0]}x{font_bbx[1]}, "
        f"{len(bdf_glyphs)} glyphs defined"
    )

    psf_data = build_psf2(font_bbx, bdf_glyphs, args.glyphs)

    Path(args.output).write_bytes(psf_data)

    print(
        f"  PSF: {args.output} "
        f"({len(psf_data)} bytes, {args.glyphs} glyphs)"
    )


if __name__ == "__main__":
    main()
