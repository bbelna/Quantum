#!/usr/bin/env python3

import ctypes

from ctypes import wintypes
from pathlib import Path

FONT_NAME_PRIMARY = "Ac437 IBM VGA 8x16"
FONT_NAME_FALLBACK = "Lucida Console"
FONT_WIDTH = 8
FONT_HEIGHT = 16
GLYPH_COUNT = 128

BI_RGB = 0
DIB_RGB_COLORS = 0
FW_NORMAL = 400
DEFAULT_CHARSET = 1
OUT_DEFAULT_PRECIS = 0
CLIP_DEFAULT_PRECIS = 0
NONANTIALIASED_QUALITY = 3
FF_DONTCARE = 0
FIXED_PITCH = 1

class BITMAPINFOHEADER(ctypes.Structure):
  _fields_ = [
    ("biSize", wintypes.DWORD),
    ("biWidth", wintypes.LONG),
    ("biHeight", wintypes.LONG),
    ("biPlanes", wintypes.WORD),
    ("biBitCount", wintypes.WORD),
    ("biCompression", wintypes.DWORD),
    ("biSizeImage", wintypes.DWORD),
    ("biXPelsPerMeter", wintypes.LONG),
    ("biYPelsPerMeter", wintypes.LONG),
    ("biClrUsed", wintypes.DWORD),
    ("biClrImportant", wintypes.DWORD),
  ]

class BITMAPINFO(ctypes.Structure):
  _fields_ = [
    ("bmiHeader", BITMAPINFOHEADER),
    ("bmiColors", wintypes.DWORD * 1),
  ]

gdi32 = ctypes.windll.gdi32
user32 = ctypes.windll.user32

def create_font(name):
  return gdi32.CreateFontW(
    -FONT_HEIGHT,
    FONT_WIDTH,
    0,
    0,
    FW_NORMAL,
    0,
    0,
    0,
    DEFAULT_CHARSET,
    OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,
    NONANTIALIASED_QUALITY,
    FIXED_PITCH | FF_DONTCARE,
    name,
  )

def write_header(path, glyphs):
  lines = []

  lines.append("#pragma once")
  lines.append("#include <Quantum/Core/Types.hpp>")
  lines.append("")
  lines.append("namespace Quantum::Services::Drivers::Graphics::Framebuffer {")
  lines.append(f"  static constexpr UInt32 kFont8x16GlyphCount = {GLYPH_COUNT};")
  lines.append(f"  static constexpr UInt32 kFont8x16Width = {FONT_WIDTH};")
  lines.append(f"  static constexpr UInt32 kFont8x16Height = {FONT_HEIGHT};")
  lines.append(
      "  static constexpr UInt8 kFont8x16[kFont8x16GlyphCount][kFont8x16Height] = {"
  )

  for glyph in glyphs:
      row = ", ".join(f"0x{value:02X}" for value in glyph)

      lines.append(f"    {{ {row} }},")

  lines.append("  };")
  lines.append("}")

  content = "\n".join(lines) + "\n"

  path.write_text(content, encoding="ascii")


def main():
  hdc = gdi32.CreateCompatibleDC(0)

  if not hdc:
    raise SystemExit("CreateCompatibleDC failed")

  bmi = BITMAPINFO()
  bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
  bmi.bmiHeader.biWidth = FONT_WIDTH
  bmi.bmiHeader.biHeight = -FONT_HEIGHT
  bmi.bmiHeader.biPlanes = 1
  bmi.bmiHeader.biBitCount = 32
  bmi.bmiHeader.biCompression = BI_RGB

  bits = ctypes.c_void_p()
  hbitmap = gdi32.CreateDIBSection(
    hdc,
    ctypes.byref(bmi),
    DIB_RGB_COLORS,
    ctypes.byref(bits),
    None,
    0,
  )

  if not hbitmap:
    gdi32.DeleteDC(hdc)

    raise SystemExit("CreateDIBSection failed")

  gdi32.SelectObject(hdc, hbitmap)

  hfont = create_font(FONT_NAME_PRIMARY)

  if not hfont:
    hfont = create_font(FONT_NAME_FALLBACK)

  if not hfont:
    gdi32.DeleteObject(hbitmap)
    gdi32.DeleteDC(hdc)

    raise SystemExit("CreateFont failed")

  gdi32.SelectObject(hdc, hfont)
  gdi32.SetBkMode(hdc, 1)
  gdi32.SetBkColor(hdc, 0x00000000)
  gdi32.SetTextColor(hdc, 0x00FFFFFF)

  buffer_size = FONT_WIDTH * FONT_HEIGHT * 4
  pixel_buffer = (ctypes.c_uint8 * buffer_size).from_address(bits.value)

  glyphs = []

  for code in range(GLYPH_COUNT):
    ctypes.memset(bits, 0, buffer_size)

    if 32 <= code < 127:
      ch = chr(code)
      gdi32.TextOutW(hdc, 0, 0, ch, 1)

    rows = []

    for row in range(FONT_HEIGHT):
      pattern = 0
      row_offset = row * FONT_WIDTH * 4

      for col in range(FONT_WIDTH):
        pixel_offset = row_offset + col * 4
        b = pixel_buffer[pixel_offset]
        g = pixel_buffer[pixel_offset + 1]
        r = pixel_buffer[pixel_offset + 2]

        if (r + g + b) > 0:
          pattern |= 1 << (7 - col)

      rows.append(pattern)

    glyphs.append(rows)

  output_path = Path(
    "Services/Drivers/Graphics/Framebuffer/Include/Font8x16.hpp"
  )

  write_header(output_path, glyphs)

  gdi32.DeleteObject(hfont)
  gdi32.DeleteObject(hbitmap)
  gdi32.DeleteDC(hdc)


if __name__ == "__main__":
  main()
