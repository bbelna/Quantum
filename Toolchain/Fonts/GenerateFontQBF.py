#!/usr/bin/env python3
"""
Converts a BDF bitmap font file to QBF (Quantum Bitmap Font) binary format.

QBF supports both monospaced and proportional bitmap fonts with per-glyph
advance widths. The output can be loaded at runtime by the Quantum Fonts
library via ParseQBF().

Usage:
  python GenerateFontQBF.py <input.bdf> <output.qbf> [--max-glyphs N]

Options:
  --max-glyphs N   Maximum glyph count (default: 256). Glyphs with
                   ENCODING >= N are skipped.
"""

import struct
import sys
from pathlib import Path

QBF_MAGIC = 0x51424631  # 'Q','B','F','1'
QBF_VERSION = 1


def parse_bdf(path, max_glyphs):
  """Parse a BDF file and return (properties, glyphs).

  properties: dict of font-level properties (FONT_ASCENT, FONT_DESCENT, etc.)
  glyphs: dict mapping encoding -> { 'dwidth': int, 'bbx': (w,h,xoff,yoff), 'rows': [int] }
  """
  properties = {}
  glyphs = {}

  encoding = None
  dwidth = None
  bbx = None
  in_bitmap = False
  in_properties = False
  rows = []

  with open(path, "r", encoding="latin-1") as f:
    for line in f:
      line = line.strip()

      if line.startswith("STARTPROPERTIES"):
        in_properties = True
        continue

      if line == "ENDPROPERTIES":
        in_properties = False
        continue

      if in_properties:
        parts = line.split(None, 1)
        if len(parts) == 2:
          key, val = parts
          # strip quotes from string values
          if val.startswith('"') and val.endswith('"'):
            properties[key] = val[1:-1]
          else:
            try:
              properties[key] = int(val)
            except ValueError:
              properties[key] = val
        continue

      if line.startswith("FONTBOUNDINGBOX "):
        parts = line.split()
        properties["_FBBX_W"] = int(parts[1])
        properties["_FBBX_H"] = int(parts[2])
        properties["_FBBX_XOFF"] = int(parts[3])
        properties["_FBBX_YOFF"] = int(parts[4])

      elif line.startswith("ENCODING "):
        encoding = int(line.split()[1])

      elif line.startswith("DWIDTH "):
        dwidth = int(line.split()[1])

      elif line.startswith("BBX "):
        parts = line.split()
        bbx = (int(parts[1]), int(parts[2]), int(parts[3]), int(parts[4]))

      elif line == "BITMAP":
        in_bitmap = True
        rows = []

      elif line == "ENDCHAR":
        if (in_bitmap and encoding is not None
            and 0 <= encoding < max_glyphs):
          glyphs[encoding] = {
            "dwidth": dwidth if dwidth is not None else 0,
            "bbx": bbx if bbx is not None else (0, 0, 0, 0),
            "rows": rows,
          }

        in_bitmap = False
        encoding = None
        dwidth = None
        bbx = None
        rows = []

      elif in_bitmap:
        rows.append(int(line, 16))

  return properties, glyphs


def build_qbf(properties, glyphs, max_glyphs):
  """Build QBF binary data from parsed BDF.

  Returns (header_bytes, glyph_data, advance_table_or_None, ascent).
  """
  ascent = properties.get("FONT_ASCENT", 0)
  descent = properties.get("FONT_DESCENT", 0)
  fbbx_w = properties.get("_FBBX_W", 8)

  # Cell height = ascent + descent (the logical line height).
  # Glyphs that extend beyond are clipped to this cell.
  cell_h = ascent + descent if (ascent + descent) > 0 else properties.get("_FBBX_H", 14)

  # Cell width = max glyph bounding box width across all glyphs.
  max_w = fbbx_w

  for enc, g in glyphs.items():
    bw, bh, _, _ = g["bbx"]
    if bw > max_w:
      max_w = bw

  cell_w = max_w
  bytes_per_row = (cell_w + 7) // 8
  bytes_per_glyph = cell_h * bytes_per_row

  # Check if proportional (any glyph has a different advance width)
  advances = []
  all_same = True
  first_adv = None

  for enc in range(max_glyphs):
    if enc in glyphs:
      adv = glyphs[enc]["dwidth"]
    else:
      adv = cell_w  # missing glyphs use cell width

    advances.append(adv)

    if first_adv is None:
      first_adv = adv
    elif adv != first_adv:
      all_same = False

  is_proportional = not all_same

  # Build glyph bitmap data
  # Each glyph is rendered into a cell_w x cell_h cell, positioned
  # according to its BBX relative to the font bounding box.
  font_descent = properties.get("FONT_DESCENT", 0)
  glyph_data = bytearray()

  for enc in range(max_glyphs):
    cell = bytearray(bytes_per_glyph)

    if enc in glyphs:
      g = glyphs[enc]
      bw, bh, bxoff, byoff = g["bbx"]
      raw_rows = g["rows"]

      # Position the glyph within the cell.
      # BDF yOffset is relative to the baseline (positive = above).
      # The baseline is at row (ascent) from the top of the cell.
      # Glyph top row = ascent - (byoff + bh)
      glyph_top = ascent - (byoff + bh)
      glyph_left = bxoff

      if glyph_left < 0:
        glyph_left = 0

      raw_bytes_per_row = (bw + 7) // 8

      for r in range(bh):
        dst_row = glyph_top + r

        if dst_row < 0 or dst_row >= cell_h:
          continue

        if r < len(raw_rows):
          raw_val = raw_rows[r]
        else:
          raw_val = 0

        # Extract individual bits from the raw row and place them
        # into the cell at the correct horizontal offset.
        for c in range(bw):
          if c >= cell_w - glyph_left:
            break

          # BDF stores MSB = leftmost pixel, multiple bytes per row
          byte_idx = c // 8
          bit_idx = 7 - (c % 8)
          raw_byte = (raw_val >> (8 * (raw_bytes_per_row - 1 - byte_idx))) & 0xFF

          if (raw_byte >> bit_idx) & 1:
            dst_col = glyph_left + c

            if dst_col >= cell_w:
              continue

            dst_byte_idx = dst_col // 8
            dst_bit_idx = 7 - (dst_col % 8)
            cell_offset = dst_row * bytes_per_row + dst_byte_idx
            cell[cell_offset] |= (1 << dst_bit_idx)

    glyph_data.extend(cell)

  # Build advance table (only if proportional)
  advance_table = None

  if is_proportional:
    advance_table = bytes(min(a, 255) for a in advances)

  return cell_w, cell_h, bytes_per_glyph, ascent, glyph_data, advance_table


def write_qbf(output_path, max_glyphs, cell_w, cell_h, bytes_per_glyph,
              ascent, glyph_data, advance_table):
  """Write QBF binary file."""
  header_size = 20
  glyph_data_offset = header_size
  glyph_data_size = len(glyph_data)

  if advance_table is not None:
    advance_table_offset = glyph_data_offset + glyph_data_size
  else:
    advance_table_offset = 0

  header = struct.pack(
    "<IBBBBHHII",
    QBF_MAGIC,
    QBF_VERSION,
    cell_w,
    cell_h,
    ascent,
    bytes_per_glyph,
    max_glyphs,
    glyph_data_offset,
    advance_table_offset,
  )

  with open(output_path, "wb") as f:
    f.write(header)
    f.write(glyph_data)

    if advance_table is not None:
      f.write(advance_table)

  total = len(header) + len(glyph_data)

  if advance_table is not None:
    total += len(advance_table)

  print(f"  wrote {output_path} ({total} bytes)")
  print(f"  {cell_w}x{cell_h} cell, {max_glyphs} glyphs, "
        f"{'proportional' if advance_table else 'monospaced'}, "
        f"ascent={ascent}")


def write_header(output_path, name, namespace, cell_w, cell_h, bytes_per_glyph,
                 ascent, max_glyphs, glyph_data, advance_table):
  """Write a C++ constexpr header with embedded font data."""
  lines = []
  lines.append(f"// Auto-generated by GenerateFontQBF.py, do not edit.")
  lines.append(f"")
  lines.append(f"#pragma once")
  lines.append(f"")
  lines.append(f"#include <Quantum/Fonts/BitmapFont.hpp>")
  lines.append(f"")
  lines.append(f"namespace {namespace} {{")

  # glyph data array
  lines.append(f"  static constexpr UInt8 {name}GlyphData[] = {{")
  for i in range(0, len(glyph_data), 16):
    chunk = glyph_data[i:i+16]
    hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
    lines.append(f"    {hex_vals},")
  lines.append(f"  }};")
  lines.append(f"")

  # advance table (if proportional)
  if advance_table is not None:
    lines.append(f"  static constexpr UInt8 {name}Advances[] = {{")
    for i in range(0, len(advance_table), 16):
      chunk = advance_table[i:i+16]
      hex_vals = ", ".join(f"{b:3d}" for b in chunk)
      lines.append(f"    {hex_vals},")
    lines.append(f"  }};")
    lines.append(f"")

  # BitmapFont instance
  adv_ref = f"{name}Advances" if advance_table else "nullptr"
  lines.append(f"  static constexpr Quantum::Fonts::BitmapFont {name} = {{")
  lines.append(f"    .GlyphData    = {name}GlyphData,")
  lines.append(f"    .Width        = {cell_w},")
  lines.append(f"    .Height       = {cell_h},")
  lines.append(f"    .BytesPerGlyph = {bytes_per_glyph},")
  lines.append(f"    .GlyphCount   = {max_glyphs},")
  lines.append(f"    .Advances     = {adv_ref},")
  lines.append(f"    .Ascent       = {ascent},")
  lines.append(f"  }};")
  lines.append(f"}}")
  lines.append(f"")

  output_path.parent.mkdir(parents=True, exist_ok=True)
  output_path.write_text("\n".join(lines), encoding="ascii")
  print(f"  wrote {output_path}")


def main():
  if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} <input.bdf> <output> [--max-glyphs N] "
          f"[--header --name NAME --namespace NS]",
          file=sys.stderr)
    sys.exit(1)

  bdf_path = Path(sys.argv[1])
  output_path = Path(sys.argv[2])
  max_glyphs = 256
  header_mode = False
  font_name = "Font"
  font_namespace = "Quantum::Fonts"

  i = 3
  while i < len(sys.argv):
    if sys.argv[i] == "--max-glyphs" and i + 1 < len(sys.argv):
      max_glyphs = int(sys.argv[i + 1])
      i += 2
    elif sys.argv[i] == "--header":
      header_mode = True
      i += 1
    elif sys.argv[i] == "--name" and i + 1 < len(sys.argv):
      font_name = sys.argv[i + 1]
      i += 2
    elif sys.argv[i] == "--namespace" and i + 1 < len(sys.argv):
      font_namespace = sys.argv[i + 1]
      i += 2
    else:
      print(f"Unknown option: {sys.argv[i]}", file=sys.stderr)
      sys.exit(1)

  if not bdf_path.exists():
    print(f"Error: BDF file not found: {bdf_path}", file=sys.stderr)
    sys.exit(1)

  print(f"Parsing {bdf_path} ...")

  properties, glyphs = parse_bdf(bdf_path, max_glyphs)

  print(f"  found {len(glyphs)} glyphs (max encoding {max_glyphs - 1})")

  cell_w, cell_h, bpg, ascent, glyph_data, adv_table = build_qbf(
    properties, glyphs, max_glyphs
  )

  print(f"Generating QBF ...")

  write_qbf(output_path, max_glyphs, cell_w, cell_h, bpg, ascent,
            glyph_data, adv_table)

  print("Done.")


if __name__ == "__main__":
  main()
