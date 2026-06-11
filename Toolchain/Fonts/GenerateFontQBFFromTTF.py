#!/usr/bin/env python3
"""
Converts a TTF/OTF font to QBF (Quantum Bitmap Font) binary format using
FreeType for high-quality hinted rasterization.

Usage:
  python3 GenerateFontQBFFromTTF.py <input.ttf> <output.qbf> <pixel_size>
         [--lcd] [--mono] [--face-index N] [--weight W]

Options:
  --lcd          Enable LCD subpixel rendering (24bpp, 3x horizontal
                 resolution). Without this flag, output is 8bpp grayscale.
  --mono         Enable monochrome rendering (1bpp, no antialiasing).
                 Mutually exclusive with --lcd.
  --face-index N Select a specific face from a TTC collection (default 0).
  --weight W     Set the weight axis of a variable font (e.g. 400=Regular,
                 500=Medium, 600=SemiBold, 700=Bold).

Example:
  python3 GenerateFontQBFFromTTF.py Inter-Regular.ttf inter12.qbf 12
  python3 GenerateFontQBFFromTTF.py Inter-Regular.ttf inter12-lcd.qbf 12 --lcd
"""

import ctypes
import ctypes.util
import struct
import sys
import os

QBF_MAGIC = 0x51424631
MAX_GLYPHS = 256

# Subpixel order constants (matches QBF v4 header)
SUBPIXEL_NONE = 0
SUBPIXEL_RGB = 1
SUBPIXEL_BGR = 2

# ---- FreeType ctypes bindings (minimal) ----

_ft_lib_name = ctypes.util.find_library("freetype")

if _ft_lib_name is None:
    # try common paths
    for candidate in ("libfreetype.so", "libfreetype.so.6"):
        try:
            _ft = ctypes.cdll.LoadLibrary(candidate)
            break
        except OSError:
            continue
    else:
        print("Error: libfreetype not found", file=sys.stderr)
        sys.exit(1)
else:
    _ft = ctypes.cdll.LoadLibrary(_ft_lib_name)


class FT_Vector(ctypes.Structure):
    _fields_ = [("x", ctypes.c_long), ("y", ctypes.c_long)]


class FT_Bitmap(ctypes.Structure):
    _fields_ = [
        ("rows", ctypes.c_int),
        ("width", ctypes.c_int),
        ("pitch", ctypes.c_int),
        ("buffer", ctypes.POINTER(ctypes.c_ubyte)),
        ("num_grays", ctypes.c_short),
        ("pixel_mode", ctypes.c_ubyte),
        ("palette_mode", ctypes.c_ubyte),
        ("palette", ctypes.c_void_p),
    ]


class FT_GlyphSlotRec(ctypes.Structure):
    pass


# We only need a few fields at known offsets
class FT_GlyphSlotRec(ctypes.Structure):
    _fields_ = [
        ("library", ctypes.c_void_p),
        ("face", ctypes.c_void_p),
        ("next", ctypes.c_void_p),
        ("glyph_index", ctypes.c_uint),
        ("generic_data", ctypes.c_void_p),
        ("generic_finalizer", ctypes.c_void_p),
        ("metrics_width", ctypes.c_long),
        ("metrics_height", ctypes.c_long),
        ("metrics_horiBearingX", ctypes.c_long),
        ("metrics_horiBearingY", ctypes.c_long),
        ("metrics_horiAdvance", ctypes.c_long),
        ("metrics_vertBearingX", ctypes.c_long),
        ("metrics_vertBearingY", ctypes.c_long),
        ("metrics_vertAdvance", ctypes.c_long),
        ("linearHoriAdvance", ctypes.c_long),
        ("linearVertAdvance", ctypes.c_long),
        ("advance", FT_Vector),
        ("format", ctypes.c_uint),
        ("bitmap", FT_Bitmap),
        ("bitmap_left", ctypes.c_int),
        ("bitmap_top", ctypes.c_int),
    ]


class FT_Size_Metrics(ctypes.Structure):
    _fields_ = [
        ("x_ppem", ctypes.c_ushort),
        ("y_ppem", ctypes.c_ushort),
        ("x_scale", ctypes.c_long),
        ("y_scale", ctypes.c_long),
        ("ascender", ctypes.c_long),
        ("descender", ctypes.c_long),
        ("height", ctypes.c_long),
        ("max_advance", ctypes.c_long),
    ]


class FT_SizeRec(ctypes.Structure):
    _fields_ = [
        ("face", ctypes.c_void_p),
        ("generic_data", ctypes.c_void_p),
        ("generic_finalizer", ctypes.c_void_p),
        ("metrics", FT_Size_Metrics),
    ]


FT_LOAD_RENDER = 4
FT_LOAD_MONOCHROME = 32
FT_LOAD_TARGET_NORMAL = 0
FT_LOAD_TARGET_MONO = (2 << 16)  # (FT_RENDER_MODE_MONO & 15) << 16
FT_LOAD_TARGET_LIGHT = (1 << 16)
FT_LOAD_TARGET_LCD = (3 << 16)
FT_PIXEL_MODE_MONO = 1
FT_PIXEL_MODE_GRAY = 2
FT_PIXEL_MODE_LCD = 5
FT_RENDER_MODE_MONO = 2
FT_RENDER_MODE_NORMAL = 0
FT_RENDER_MODE_LCD = 3


class FT_Fixed(ctypes.c_long):
    """FreeType 16.16 fixed-point type."""
    pass


class FT_Var_Axis(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("minimum", FT_Fixed),
        ("def_", FT_Fixed),
        ("maximum", FT_Fixed),
        ("tag", ctypes.c_ulong),
        ("strid", ctypes.c_uint),
    ]


class FT_MM_Var(ctypes.Structure):
    _fields_ = [
        ("num_axis", ctypes.c_uint),
        ("num_designs", ctypes.c_uint),
        ("num_namedstyles", ctypes.c_uint),
        ("axis", ctypes.POINTER(FT_Var_Axis)),
        # further fields omitted
    ]


def set_variable_font_weight(face, weight):
    """Set the weight axis on a variable font.

    weight is a CSS-style value (e.g. 400=Regular, 500=Medium,
    600=SemiBold, 700=Bold).
    """
    mm_var_ptr = ctypes.POINTER(FT_MM_Var)()
    err = _ft.FT_Get_MM_Var(face, ctypes.byref(mm_var_ptr))

    if err:
        print(f"  Warning: FT_Get_MM_Var failed ({err}), "
              f"--weight ignored (not a variable font?)",
              file=sys.stderr)
        return

    mm_var = mm_var_ptr.contents
    num_axes = mm_var.num_axis

    # Build coordinate array with default values
    coords = (FT_Fixed * num_axes)()

    for i in range(num_axes):
        coords[i] = mm_var.axis[i].def_

    # Find and set the weight axis (tag 'wght' = 0x77676874)
    wght_tag = 0x77676874
    found = False

    for i in range(num_axes):
        if mm_var.axis[i].tag == wght_tag:
            coords[i] = FT_Fixed(weight << 16)  # 16.16 fixed point
            found = True
            break

    if not found:
        print(f"  Warning: no 'wght' axis found, --weight ignored",
              file=sys.stderr)
        return

    err = _ft.FT_Set_Var_Design_Coordinates(face, num_axes, coords)

    if err:
        print(f"  Warning: FT_Set_Var_Design_Coordinates failed ({err})",
              file=sys.stderr)
    else:
        print(f"  Set variable font weight axis to {weight}")


def parse_args():
    """Parse command-line arguments."""
    positional = []
    lcd_mode = False
    mono_mode = False
    face_index = 0
    weight = None

    i = 1
    while i < len(sys.argv):
        arg = sys.argv[i]

        if arg == "--lcd":
            lcd_mode = True
        elif arg == "--mono":
            mono_mode = True
        elif arg == "--face-index":
            i += 1

            if i >= len(sys.argv):
                print("Error: --face-index requires a value", file=sys.stderr)
                sys.exit(1)

            face_index = int(sys.argv[i])
        elif arg == "--weight":
            i += 1

            if i >= len(sys.argv):
                print("Error: --weight requires a value", file=sys.stderr)
                sys.exit(1)

            weight = int(sys.argv[i])
        else:
            positional.append(arg)

        i += 1

    if len(positional) < 3:
        print(f"Usage: {sys.argv[0]} <input.ttf> <output.qbf> <pixel_size> "
              f"[--lcd] [--mono] [--face-index N] [--weight W]",
              file=sys.stderr)
        sys.exit(1)

    if lcd_mode and mono_mode:
        print("Error: --lcd and --mono are mutually exclusive", file=sys.stderr)
        sys.exit(1)

    return (positional[0], positional[1], int(positional[2]),
            lcd_mode, mono_mode, face_index, weight)


def read_glyph_slot(face):
    """Read the glyph slot pointer from a FreeType face struct."""
    ptr_size = ctypes.sizeof(ctypes.c_void_p)

    # Empirically on x86_64 Linux with FreeType 2.x:
    # face->glyph is at offset 152 bytes from face pointer
    # On 32-bit it would differ.
    glyph_slot_ptr = ctypes.c_void_p.from_address(
        face.value + 152 if ptr_size == 8 else face.value + 76
    ).value

    if not glyph_slot_ptr:
        return None

    return FT_GlyphSlotRec.from_address(glyph_slot_ptr)


def build_smoothing_lut(gamma=0.75):
    """Build a coverage-boost lookup table (Apple-style font smoothing).

    Applies a power curve to coverage values so that mid-range values
    are pushed darker, giving stems a heavier appearance similar to
    macOS Core Text rendering.  The default gamma of 0.75 closely
    matches the "Medium" font smoothing setting in macOS.

    gamma < 1.0 = heavier (darker stems), gamma > 1.0 = lighter.
    """
    lut = bytearray(256)

    for i in range(256):
        lut[i] = min(255, int(255.0 * pow(i / 255.0, gamma) + 0.5))

    return bytes(lut)


# LCD subpixel fonts have 3x horizontal resolution so stems are already
# sharp - a mild gamma of 0.80 is enough to match macOS "Medium" smoothing.
# Grayscale fonts lack that extra resolution so stem pixels that land between
# grid lines end up with low coverage (often 140-200).  A much more
# aggressive gamma of 0.45 pushes those values close to solid black while
# still keeping edge anti-aliasing smooth.
_SMOOTHING_LUT_LCD = build_smoothing_lut(0.80)
_SMOOTHING_LUT_GRAY = build_smoothing_lut(0.45)


def apply_smoothing(row_bytes, lut):
    """Apply a font smoothing LUT to a row of coverage bytes."""
    return bytes(lut[b] for b in row_bytes)


def rasterize_glyphs(face, lcd_mode, mono_mode):
    """Rasterize all 256 glyphs from the given FreeType face.

    Returns a dict: enc -> (bitmap_left, bitmap_top, pixel_width,
                            rows, advance, row_data)
    where pixel_width is the width in actual pixels (not subpixel samples)
    and row_data contains either 1 byte/pixel (grayscale/mono) or
    3 bytes/pixel (LCD R,G,B) per row.  Mono glyphs are unpacked to
    one byte per pixel (0x00 or 0xFF) for uniform blitting.
    """
    # Light hinting preserves stem shapes faithfully at small sizes
    # (normal hinting snaps too aggressively and distorts proportions).
    # The smoothing gamma LUT compensates for the lower coverage values
    # that light hinting produces.
    # For LCD mode, load with light hinting first, then render as LCD
    # separately - this avoids TARGET_LCD's aggressive grid-snapping
    # which causes inconsistent stem darkness.
    if mono_mode:
        load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME
    elif lcd_mode:
        load_flags = FT_LOAD_TARGET_LIGHT
    else:
        load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT

    glyphs = {}

    for enc in range(MAX_GLYPHS):
        glyph_index_fn = _ft.FT_Get_Char_Index
        glyph_index_fn.restype = ctypes.c_uint
        glyph_idx = glyph_index_fn(face, enc)

        if glyph_idx == 0 and enc != 0:
            continue

        err = _ft.FT_Load_Char(face, enc, load_flags)

        if err:
            continue

        if lcd_mode:
            ptr_size = ctypes.sizeof(ctypes.c_void_p)
            slot_ptr = ctypes.c_void_p.from_address(
                face.value + (152 if ptr_size == 8 else 76)
            ).value

            if not slot_ptr:
                continue

            render_fn = _ft.FT_Render_Glyph
            render_fn.argtypes = [ctypes.c_void_p, ctypes.c_int]
            render_fn.restype = ctypes.c_int
            err = render_fn(slot_ptr, FT_RENDER_MODE_LCD)

            if err:
                continue

        if err:
            continue

        slot = read_glyph_slot(face)

        if slot is None:
            continue

        bmp = slot.bitmap
        advance = (slot.metrics_horiAdvance + 32) >> 6  # 26.6 fixed point

        if lcd_mode:
            # LCD bitmap: width is 3x pixel width (one byte per subpixel)
            pixel_width = bmp.width // 3
        else:
            pixel_width = bmp.width

        if pixel_width == 0 or bmp.rows == 0:
            # Space character or similar
            glyphs[enc] = (0, 0, 0, 0, advance, [])
            continue

        row_data = []

        if mono_mode:
            # Unpack 1-bit FreeType bitmap to one byte per pixel
            for r in range(bmp.rows):
                row_start = r * abs(bmp.pitch)
                row_bytes = []

                for c in range(pixel_width):
                    byte_idx = c // 8
                    bit_idx = 7 - (c % 8)
                    val = bmp.buffer[row_start + byte_idx]
                    row_bytes.append(255 if (val >> bit_idx) & 1 else 0)

                row_data.append(bytes(row_bytes))
        else:
            for r in range(bmp.rows):
                row_start = r * abs(bmp.pitch)
                row_bytes = bytes(
                    bmp.buffer[row_start : row_start + bmp.width]
                )

                lut = _SMOOTHING_LUT_LCD if lcd_mode else _SMOOTHING_LUT_GRAY
                row_data.append(apply_smoothing(row_bytes, lut))

        glyphs[enc] = (
            slot.bitmap_left,
            slot.bitmap_top,
            pixel_width,
            bmp.rows,
            advance,
            row_data,
        )

    return glyphs


def compute_cell_metrics(glyphs, pixel_size):
    """Compute cell dimensions from rasterized glyphs.

    Returns (cell_w, cell_h, ascent_px).
    """
    max_ascent = 0
    max_descent = 0
    max_width = 0

    for enc, (bleft, btop, bw, brows, adv, _) in glyphs.items():
        if btop > max_ascent:
            max_ascent = btop

        descent = brows - btop

        if descent > max_descent:
            max_descent = descent

        right_extent = bleft + bw

        if right_extent > max_width:
            max_width = right_extent

        if adv > max_width:
            max_width = adv

    cell_h = max_ascent + max_descent
    cell_w = max_width

    if cell_h <= 0:
        cell_h = pixel_size

    if cell_w <= 0:
        cell_w = pixel_size

    return cell_w, cell_h, max_ascent


def build_glyph_data(glyphs, cell_w, cell_h, ascent_px, lcd_mode, mono_mode):
    """Pack glyph bitmaps into fixed-size cells.

    Returns (glyph_data, advances, is_proportional, bytes_per_glyph).
    """
    if mono_mode:
        bytes_per_row = (cell_w + 7) // 8
        bytes_per_glyph = cell_h * bytes_per_row
    else:
        bytes_per_pixel = 3 if lcd_mode else 1
        bytes_per_glyph = cell_h * cell_w * bytes_per_pixel

    glyph_data = bytearray()
    advances = []
    all_same = True
    first_adv = None

    for enc in range(MAX_GLYPHS):
        cell = bytearray(bytes_per_glyph)

        if enc in glyphs:
            bleft, btop, bw, brows, adv, row_data = glyphs[enc]

            glyph_top = ascent_px - btop
            glyph_left = bleft

            for r in range(brows):
                dst_row = glyph_top + r

                if dst_row < 0 or dst_row >= cell_h:
                    continue

                if r >= len(row_data):
                    continue

                for c in range(bw):
                    dst_col = glyph_left + c

                    if dst_col < 0 or dst_col >= cell_w:
                        continue

                    if mono_mode:
                        val = (row_data[r][c]
                               if c < len(row_data[r]) else 0)

                        if val:
                            byte_idx = dst_col // 8
                            bit_idx = 7 - (dst_col % 8)
                            cell[dst_row * bytes_per_row + byte_idx] \
                                |= (1 << bit_idx)
                    elif lcd_mode:
                        src_offset = c * 3
                        dst_offset = (dst_row * cell_w + dst_col) * 3

                        for ch in range(3):
                            if src_offset + ch < len(row_data[r]):
                                cell[dst_offset + ch] = \
                                    row_data[r][src_offset + ch]
                    else:
                        gray = (row_data[r][c]
                                if c < len(row_data[r]) else 0)
                        cell[dst_row * cell_w + dst_col] = gray

            advances.append(min(adv, 255))
        else:
            advances.append(cell_w)

        if first_adv is None:
            first_adv = advances[-1]
        elif advances[-1] != first_adv:
            all_same = False

        glyph_data.extend(cell)

    is_proportional = not all_same
    advance_table = bytes(advances) if is_proportional else None

    return glyph_data, advance_table, is_proportional, bytes_per_glyph


def compute_cap_metrics(glyphs, ascent_px, cell_h):
    """Compute cap-height metrics from the 'H' glyph."""
    h_enc = ord('H')

    if h_enc in glyphs:
        _, h_btop, _, h_rows, _, _ = glyphs[h_enc]
        cap_top = ascent_px - h_btop
        cap_height = h_rows

        if cap_top < 0:
            cap_top = 0

        if cap_top + cap_height > cell_h:
            cap_height = cell_h - cap_top
    else:
        cap_top = 0
        cap_height = ascent_px

    return cap_top, cap_height


def write_qbf(output_path, cell_w, cell_h, ascent_px, bytes_per_glyph,
              glyph_data, advance_table, cap_top, cap_height,
              lcd_mode, mono_mode):
    """Write the QBF file."""
    if lcd_mode:
        bits_per_pixel = 24
        version = 4
    elif mono_mode:
        bits_per_pixel = 1
        version = 3
    else:
        bits_per_pixel = 8
        version = 3

    # Header sizes: 20-byte base + 1-byte v2 + 2-byte v3 [+ 1-byte v4]
    header_size = 23

    if version >= 4:
        header_size += 1  # v4 ext

    glyph_data_offset = header_size
    advance_table_offset = (glyph_data_offset + len(glyph_data)
                            if advance_table else 0)

    header = struct.pack(
        "<IBBBBHHII",
        QBF_MAGIC,
        version,
        cell_w,
        cell_h,
        ascent_px,
        bytes_per_glyph,
        MAX_GLYPHS,
        glyph_data_offset,
        advance_table_offset,
    )

    v2_ext = struct.pack("<B", bits_per_pixel)
    v3_ext = struct.pack("<BB", cap_top, cap_height)

    with open(output_path, "wb") as f:
        f.write(header)
        f.write(v2_ext)
        f.write(v3_ext)

        if version >= 4:
            v4_ext = struct.pack("<B", SUBPIXEL_RGB)
            f.write(v4_ext)

        f.write(glyph_data)

        if advance_table:
            f.write(advance_table)

    total = header_size + len(glyph_data)

    if advance_table:
        total += len(advance_table)

    if lcd_mode:
        mode_str = "24bpp LCD (RGB)"
    elif mono_mode:
        mode_str = "1bpp monochrome"
    else:
        mode_str = "8bpp grayscale"
    prop_str = "proportional" if advance_table else "monospaced"

    print(f"  Wrote {output_path} ({total} bytes)")
    print(f"  {cell_w}x{cell_h} cell, {MAX_GLYPHS} glyphs, {mode_str}, "
          f"{prop_str}, ascent={ascent_px}")


def main():
    (ttf_path, output_path, pixel_size,
     lcd_mode, mono_mode, face_index, weight) = parse_args()

    # Initialize FreeType
    library = ctypes.c_void_p()
    err = _ft.FT_Init_FreeType(ctypes.byref(library))

    if err:
        print(f"FT_Init_FreeType failed: {err}", file=sys.stderr)
        sys.exit(1)

    # Load face
    face = ctypes.c_void_p()
    path_bytes = ttf_path.encode("utf-8")
    err = _ft.FT_New_Face(library, path_bytes, face_index, ctypes.byref(face))

    if err:
        print(f"FT_New_Face failed: {err}", file=sys.stderr)
        sys.exit(1)

    # Set variable font weight axis if requested
    if weight is not None:
        set_variable_font_weight(face, weight)

    # Set pixel size
    err = _ft.FT_Set_Pixel_Sizes(face, 0, pixel_size)

    if err:
        print(f"FT_Set_Pixel_Sizes failed: {err}", file=sys.stderr)
        sys.exit(1)

    # Rasterize
    if lcd_mode:
        mode_name = "LCD subpixel"
    elif mono_mode:
        mode_name = "monochrome"
    else:
        mode_name = "grayscale"

    print(f"  Rasterizing {mode_name} at {pixel_size}px...")

    glyphs = rasterize_glyphs(face, lcd_mode, mono_mode)

    _ft.FT_Done_Face(face)
    _ft.FT_Done_FreeType(library)

    print(f"  Rasterized {len(glyphs)} glyphs")

    if not glyphs:
        print("Error: no glyphs rasterized", file=sys.stderr)
        sys.exit(1)

    # Compute metrics
    cell_w, cell_h, ascent_px = compute_cell_metrics(glyphs, pixel_size)

    print(f"  cell_w={cell_w} cell_h={cell_h} ascent={ascent_px}")

    # Build glyph data
    glyph_data, advance_table, is_proportional, bytes_per_glyph = \
        build_glyph_data(
            glyphs, cell_w, cell_h, ascent_px, lcd_mode, mono_mode
        )

    # Cap-height
    cap_top, cap_height = compute_cap_metrics(glyphs, ascent_px, cell_h)
    print(f"  capTop={cap_top} capHeight={cap_height}")

    # Write output
    write_qbf(
        output_path, cell_w, cell_h, ascent_px, bytes_per_glyph,
        glyph_data, advance_table, cap_top, cap_height,
        lcd_mode, mono_mode
    )

    print("Done.")


if __name__ == "__main__":
    main()
