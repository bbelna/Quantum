/**
 * @file Kernel/Platform/PC/Drivers/Graphics/VGADriver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Graphics::VGADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Memory/E820BootInfo.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>
#include <Drivers/CPU/ICPUDriver.hpp>
#include <KernelLog.hpp>
#include <KernelFont.hpp>
#include <Memory/IMemoryAllocator.hpp>
#include <Memory/IMemoryMapper.hpp>
#include <Memory/MemoryMappingFlags.hpp>
#include <Memory/SharedBufferRepository.hpp>

#include "VGADriver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  VGADriver::VGADriver(ICPUDriver& cpu, IMaydayHandler* maydayHandler)
    : _cpu(cpu), _maydayHandler(maydayHandler) {
    _disableHardwareCursor();
    Clear();

    MAYDAY("VGA is currently unsupported. See issue #65 for details.");
  }

  void VGADriver::WriteCharacter(char character) {
    if (_graphicsMode) return;

    _lock.Acquire();

    _hideCursor();

    switch (character) {
      case '\n': {
        if (_cursorState.Column != 0) {
          _cursorState.Column = 0;
          _cursorState.Row++;
        }

        break;
      } case '\r': {
        _cursorState.Column = 0;

        break;
      } case '\b': {
        if (_cursorState.Column > 0) {
          _cursorState.Column--;
          _buffer[_index(_cursorState.Row, _cursorState.Column)]
            = _makeEntry(' ', _color);
        } else if (_cursorState.Row > 0) {
          _cursorState.Row--;
          _cursorState.Column = _columns - 1;
          _buffer[_index(_cursorState.Row, _cursorState.Column)]
            = _makeEntry(' ', _color);
        }

        break;
      } default: {
        UInt16 entry = _makeEntry(character, _color);

        _buffer[_index(_cursorState.Row, _cursorState.Column)] = entry;
        _cursorState.Column++;

        if (_cursorState.Column >= _columns) {
          _cursorState.Column = 0;
          _cursorState.Row++;
        }

        break;
      }
    }

    if (_cursorState.Row >= _rows) {
      for (UInt16 row = 1; row < _rows; ++row) {
        for (UInt16 column = 0; column < _columns; ++column) {
          _buffer[_index(row - 1, column)] = _buffer[_index(row, column)];
        }
      }

      UInt16 blank = _makeEntry(' ', _color);

      for (UInt16 column = 0; column < _columns; ++column) {
        _buffer[_index(_rows - 1, column)] = blank;
      }

      _cursorState.Row = _rows - 1;
      _cursorState.Column = 0;
      _cursorState.Visible = false;
      _cursorState.Cell = _makeEntry(' ', _color);
    }

    _drawCursor();

    _lock.Release();
  }

  void VGADriver::WriteText(const char* string) {
    while (*string) WriteCharacter(*string++);
  }

  void VGADriver::Clear() {
    _lock.Acquire();

    _hideCursor();

    UInt16 blank = _makeEntry(' ', _color);

    for (UInt16 row = 0; row < _rows; ++row) {
      for (UInt16 column = 0; column < _columns; ++column) {
        _buffer[_index(row, column)] = blank;
      }
    }

    _cursorState.Row = 0;
    _cursorState.Column = 0;

    _drawCursor();

    _lock.Release();
  }

  void VGADriver::SetColor(UInt8 foreground, UInt8 background) {
    _lock.Acquire();

    _hideCursor();

    _color = (background << 4) | (foreground & 0x0F);

    _drawCursor();

    _lock.Release();
  }

  void VGADriver::SetTextCursorPosition(Geometry2D::Point position) {
    _lock.Acquire();

    _hideCursor();

    if (position.X < _columns) _cursorState.Column = position.X;
    if (position.Y < _rows) _cursorState.Row = position.Y;

    _drawCursor();

    _lock.Release();
  }

  void VGADriver::_disableHardwareCursor() {
    _cpu.Out8(0x3D4, 0x0A);
    UInt8 cursorStart = _cpu.In8(0x3D5);

    cursorStart |= 0x20; // bit 5 disables the cursor

    _cpu.Out8(0x3D4, 0x0A);
    _cpu.Out8(0x3D5, cursorStart);
  }

  void VGADriver::_drawCursor() {
    _cursorState.Cell
      = _buffer[_index(_cursorState.Row, _cursorState.Column)];

    UInt8 character = _cursorState.Cell & 0xFF;
    UInt16 blockCell = _makeEntry(static_cast<char>(character), 0xF0);

    _buffer[_index(_cursorState.Row, _cursorState.Column)] = blockCell;
    _cursorState.Visible = true;
  }

  void VGADriver::_hideCursor() {
    if (_cursorState.Visible) {
      _buffer[_index(_cursorState.Row, _cursorState.Column)]
        = _cursorState.Cell;
      _cursorState.Visible = false;
    }
  }

  // -------------------------------------------------------------------------
  // Graphics mode
  // -------------------------------------------------------------------------

  // Standard 16-color CGA/VGA palette (DAC values, 6-bit: 0-63)
  static constexpr UInt8 StandardPalette[16][3] = {
    {  0,  0,  0 },  // 0  Black
    {  0,  0, 42 },  // 1  Blue
    {  0, 42,  0 },  // 2  Green
    {  0, 42, 42 },  // 3  Cyan
    { 42,  0,  0 },  // 4  Red
    { 42,  0, 42 },  // 5  Magenta
    { 42, 21,  0 },  // 6  Brown
    { 42, 42, 42 },  // 7  Light Gray
    { 21, 21, 21 },  // 8  Dark Gray
    { 21, 21, 63 },  // 9  Light Blue
    { 21, 63, 21 },  // 10 Light Green
    { 21, 63, 63 },  // 11 Light Cyan
    { 63, 21, 21 },  // 12 Light Red
    { 63, 21, 63 },  // 13 Light Magenta
    { 63, 63, 21 },  // 14 Yellow
    { 63, 63, 63 },  // 15 White
  };

  // --- Mode 12h: 640x480, 16 colors, planar ---

  static constexpr UInt8 Mode12hMiscOutput = 0xE3;

  static constexpr UInt8 Mode12hSequencer[5] = {
    0x03, 0x01, 0x0F, 0x00, 0x06
  };

  static constexpr UInt8 Mode12hCRTC[25] = {
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
    0xFF
  };

  static constexpr UInt8 Mode12hGraphicsController[9] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F,
    0xFF
  };

  static constexpr UInt8 Mode12hAttributeController[21] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x01, 0x00, 0x0F, 0x00, 0x00
  };

  // --- Mode 13h: 320x200, 256 colors, linear ---

  static constexpr UInt8 Mode13hMiscOutput = 0x63;

  static constexpr UInt8 Mode13hSequencer[5] = {
    0x03, 0x01, 0x0F, 0x00, 0x0E
  };

  static constexpr UInt8 Mode13hCRTC[25] = {
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF
  };

  static constexpr UInt8 Mode13hGraphicsController[9] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF
  };

  static constexpr UInt8 Mode13hAttributeController[21] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
  };

  // --- Common register programming ---

  void VGADriver::_programVGARegisters(
    UInt8 miscOutput,
    const UInt8* seq,
    const UInt8* crtc,
    const UInt8* gc,
    const UInt8* ac
  ) {
    _cpu.Out8(0x3C2, miscOutput);

    for (UInt8 i = 0; i < 5; ++i) {
      _cpu.Out8(0x3C4, i);
      _cpu.Out8(0x3C5, seq[i]);
    }

    // unlock CRTC registers
    _cpu.Out8(0x3D4, 0x11);
    _cpu.Out8(0x3D5, _cpu.In8(0x3D5) & ~0x80);

    for (UInt8 i = 0; i < 25; ++i) {
      _cpu.Out8(0x3D4, i);
      _cpu.Out8(0x3D5, crtc[i]);
    }

    for (UInt8 i = 0; i < 9; ++i) {
      _cpu.Out8(0x3CE, i);
      _cpu.Out8(0x3CF, gc[i]);
    }

    _cpu.In8(0x3DA);

    for (UInt8 i = 0; i < 21; ++i) {
      _cpu.Out8(0x3C0, i);
      _cpu.Out8(0x3C0, ac[i]);
    }

    _cpu.Out8(0x3C0, 0x20);
  }

  void VGADriver::SetMode(UInt16 mode) {
    if (mode != 0x12 && mode != 0x13) return;

    _lock.Acquire();

    if (mode == 0x12) {
      _programVGARegisters(
        Mode12hMiscOutput, Mode12hSequencer, Mode12hCRTC,
        Mode12hGraphicsController, Mode12hAttributeController
      );

      _gfxWidth = 640;
      _gfxHeight = 480;
      _bytesPerRow = 80;
      _planarMode = true;
    } else {
      _programVGARegisters(
        Mode13hMiscOutput, Mode13hSequencer, Mode13hCRTC,
        Mode13hGraphicsController, Mode13hAttributeController
      );

      _gfxWidth = 320;
      _gfxHeight = 200;
      _bytesPerRow = 320;
      _planarMode = false;
    }

    // program the DAC with the standard 16-color palette
    _cpu.Out8(0x3C8, 0);

    for (UInt8 i = 0; i < 16; ++i) {
      _cpu.Out8(0x3C9, StandardPalette[i][0]);
      _cpu.Out8(0x3C9, StandardPalette[i][1]);
      _cpu.Out8(0x3C9, StandardPalette[i][2]);
    }

    // clear the framebuffer
    if (_planarMode) {
      // write mode 0, enable Set/Reset for all planes, color = 0
      _cpu.Out8(0x3CE, 5); _cpu.Out8(0x3CF, 0x00);
      _cpu.Out8(0x3CE, 1); _cpu.Out8(0x3CF, 0x0F);
      _cpu.Out8(0x3CE, 0); _cpu.Out8(0x3CF, 0x00);
      _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, 0xFF);

      UInt32 total = static_cast<UInt32>(_bytesPerRow) * _gfxHeight;

      for (UInt32 i = 0; i < total; ++i) {
        _framebuffer[i] = 0x00;
      }

      _cpu.Out8(0x3CE, 1); _cpu.Out8(0x3CF, 0x00);
    } else {
      UInt32 total = static_cast<UInt32>(_gfxWidth) * _gfxHeight;

      for (UInt32 i = 0; i < total; ++i) {
        _framebuffer[i] = 0;
      }
    }

    _graphicsMode = true;

    _lock.Release();
  }

  // --- Planar mode helpers (Mode 12h) ---

  void VGADriver::FillRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    if (!_graphicsMode) return;

    _lock.Acquire();

    UInt8 paletteIndex = _toPaletteIndex(color);

    UInt16 x2 = x + w;
    UInt16 y2 = y + h;

    if (x2 > _gfxWidth) x2 = _gfxWidth;
    if (y2 > _gfxHeight) y2 = _gfxHeight;
    if (x >= _gfxWidth || y >= _gfxHeight) { _lock.Release(); return; }

    if (_planarMode) {
      // use write mode 0 with Set/Reset to fill with a solid color
      _cpu.Out8(0x3CE, 5); _cpu.Out8(0x3CF, 0x00); // write mode 0
      _cpu.Out8(0x3CE, 1); _cpu.Out8(0x3CF, 0x0F); // enable Set/Reset all planes
      _cpu.Out8(0x3CE, 0); _cpu.Out8(0x3CF, paletteIndex); // Set/Reset value
      _cpu.Out8(0x3CE, 3); _cpu.Out8(0x3CF, 0x00); // data rotate = 0

      for (UInt16 row = y; row < y2; ++row) {
        UInt32 rowBase = static_cast<UInt32>(row) * _bytesPerRow;
        UInt16 startByte = x / 8;
        UInt16 endByte = (x2 - 1) / 8;
        UInt8 startMask = 0xFF >> (x & 7);
        UInt8 endMask =
          static_cast<UInt8>(0xFF << (7 - ((x2 - 1) & 7)));

        if (startByte == endByte) {
          UInt8 mask = startMask & endMask;
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, mask);
          volatile UInt8 latch = _framebuffer[rowBase + startByte];
          (void)latch;
          _framebuffer[rowBase + startByte] = 0xFF;
        } else {
          // first partial byte
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, startMask);
          volatile UInt8 latch = _framebuffer[rowBase + startByte];
          (void)latch;
          _framebuffer[rowBase + startByte] = 0xFF;

          // full middle bytes (no latch read needed: mask 0xFF means
          // latch is ANDed with 0x00, contributing nothing)
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, 0xFF);

          for (UInt16 b = startByte + 1; b < endByte; ++b) {
            _framebuffer[rowBase + b] = 0xFF;
          }

          // last partial byte
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, endMask);
          latch = _framebuffer[rowBase + endByte];
          _framebuffer[rowBase + endByte] = 0xFF;
        }
      }

      // restore defaults
      _cpu.Out8(0x3CE, 1); _cpu.Out8(0x3CF, 0x00);
      _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, 0xFF);
    } else {
      // linear mode (Mode 13h)
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 rowOffset = static_cast<UInt32>(row) * _gfxWidth;

        for (UInt16 col = x; col < x2; ++col) {
          _framebuffer[rowOffset + col] = paletteIndex;
        }
      }
    }

    _lock.Release();
  }

  void VGADriver::BlitBuffer(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 transparentColor,
    const UInt32* pixels
  ) {
    if (!_graphicsMode || !pixels) return;

    _lock.Acquire();

    UInt8 transparentIndex = _toPaletteIndex(transparentColor);

    if (_planarMode) {
      // use write mode 2: write color directly, Bit Mask selects pixel
      _cpu.Out8(0x3CE, 5); _cpu.Out8(0x3CF, 0x02); // write mode 2
      _cpu.Out8(0x3CE, 3); _cpu.Out8(0x3CF, 0x00); // data rotate = 0

      UInt16 endX = x + w;
      if (endX > _gfxWidth) endX = _gfxWidth;

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _gfxHeight) break;

        UInt32 rowBase = static_cast<UInt32>(screenY) * _bytesPerRow;
        UInt32 srcOffset = static_cast<UInt32>(row) * w;

        // process pixels byte-by-byte, grouping same-color bitmasks
        UInt16 startByte = x / 8;
        UInt16 lastByte = (endX - 1) / 8;

        for (UInt16 b = startByte; b <= lastByte; ++b) {
          UInt8 masks[16] = {};
          bool anyPixel = false;

          UInt16 bitStart = (b == startByte) ? (x & 7) : 0;
          UInt16 bitEnd = (b == lastByte) ? ((endX - 1) & 7) + 1 : 8;

          for (UInt16 bit = bitStart; bit < bitEnd; ++bit) {
            UInt16 col = b * 8 + bit - x;

            if (col >= w) break;

            UInt8 pixel = _toPaletteIndex(pixels[srcOffset + col]);

            if (pixel != transparentIndex) {
              masks[pixel] |= static_cast<UInt8>(0x80 >> bit);
              anyPixel = true;
            }
          }

          if (!anyPixel) continue;

          UInt32 byteOffset = rowBase + b;

          for (UInt8 c = 0; c < 16; ++c) {
            if (masks[c]) {
              _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, masks[c]);
              volatile UInt8 latch = _framebuffer[byteOffset];
              (void)latch;
              _framebuffer[byteOffset] = c;
            }
          }
        }
      }

      // restore defaults
      _cpu.Out8(0x3CE, 5); _cpu.Out8(0x3CF, 0x00);
      _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, 0xFF);
    } else {
      // linear mode (Mode 13h)
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _gfxHeight) break;

        UInt32 rowOffset = static_cast<UInt32>(screenY) * _gfxWidth;
        UInt32 srcOffset = static_cast<UInt32>(row) * w;

        for (UInt16 col = 0; col < w; ++col) {
          UInt16 screenX = x + col;

          if (screenX >= _gfxWidth) break;

          UInt8 pixel = _toPaletteIndex(pixels[srcOffset + col]);

          if (pixel != transparentIndex) {
            _framebuffer[rowOffset + screenX] = pixel;
          }
        }
      }
    }

    _lock.Release();
  }

  void VGADriver::XORRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    if (!_graphicsMode) return;

    _lock.Acquire();

    UInt8 paletteIndex = _toPaletteIndex(color);

    UInt16 x2 = x + w;
    UInt16 y2 = y + h;

    if (x2 > _gfxWidth) x2 = _gfxWidth;
    if (y2 > _gfxHeight) y2 = _gfxHeight;
    if (x >= _gfxWidth || y >= _gfxHeight) { _lock.Release(); return; }

    if (_planarMode) {
      // write mode 0 with Set/Reset + XOR logical operation
      _cpu.Out8(0x3CE, 5); _cpu.Out8(0x3CF, 0x00); // write mode 0
      _cpu.Out8(0x3CE, 1); _cpu.Out8(0x3CF, 0x0F); // enable Set/Reset all planes
      _cpu.Out8(0x3CE, 0); _cpu.Out8(0x3CF, paletteIndex); // Set/Reset value
      _cpu.Out8(0x3CE, 3); _cpu.Out8(0x3CF, 0x18); // data rotate = XOR

      for (UInt16 row = y; row < y2; ++row) {
        UInt32 rowBase = static_cast<UInt32>(row) * _bytesPerRow;
        UInt16 startByte = x / 8;
        UInt16 endByte = (x2 - 1) / 8;
        UInt8 startMask = 0xFF >> (x & 7);
        UInt8 endMask =
          static_cast<UInt8>(0xFF << (7 - ((x2 - 1) & 7)));

        if (startByte == endByte) {
          UInt8 mask = startMask & endMask;
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, mask);
          volatile UInt8 latch = _framebuffer[rowBase + startByte];
          (void)latch;
          _framebuffer[rowBase + startByte] = 0xFF;
        } else {
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, startMask);
          volatile UInt8 latch = _framebuffer[rowBase + startByte];
          (void)latch;
          _framebuffer[rowBase + startByte] = 0xFF;

          // latch reads ARE needed for XOR (result depends on existing data)
          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, 0xFF);

          for (UInt16 b = startByte + 1; b < endByte; ++b) {
            latch = _framebuffer[rowBase + b];
            _framebuffer[rowBase + b] = 0xFF;
          }

          _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, endMask);
          latch = _framebuffer[rowBase + endByte];
          _framebuffer[rowBase + endByte] = 0xFF;
        }
      }

      // restore defaults
      _cpu.Out8(0x3CE, 1); _cpu.Out8(0x3CF, 0x00);
      _cpu.Out8(0x3CE, 3); _cpu.Out8(0x3CF, 0x00);
      _cpu.Out8(0x3CE, 8); _cpu.Out8(0x3CF, 0xFF);
    } else {
      // linear mode: XOR each pixel
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 rowOffset = static_cast<UInt32>(row) * _gfxWidth;

        for (UInt16 col = x; col < x2; ++col) {
          _framebuffer[rowOffset + col] ^= paletteIndex;
        }
      }
    }

    _lock.Release();
  }

  void VGADriver::GetModeInfo(ModeInfoPayload* info) {
    if (!info) return;

    info->Width = _gfxWidth;
    info->Height = _gfxHeight;
    info->BitsPerPixel = _planarMode ? 4 : 8;
    info->Pitch = _bytesPerRow;
  }
}
