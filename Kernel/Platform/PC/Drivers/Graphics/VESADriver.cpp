/**
 * @file Kernel/Platform/PC/Drivers/Graphics/VESADriver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Graphics::VESADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Memory/E820BootInfo.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>
#include <KernelLog.hpp>
#include <KernelFont.hpp>
#include <Memory/IMemoryAllocator.hpp>
#include <Memory/IMemoryMapper.hpp>
#include <Memory/MemoryMappingFlags.hpp>
#include <Memory/SharedBufferRepository.hpp>

#include "VESADriver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  static UInt16 _toRGB565(UInt32 argb) {
    return static_cast<UInt16>(
      ((argb >> 8) & 0xF800) |
      ((argb >> 5) & 0x07E0) |
      ((argb >> 3) & 0x001F)
    );
  }

  UInt32 VESADriver::_clearPattern() const {
    if (_bytesPerPixel == 4) {
      return ClearColor;
    } else if (_bytesPerPixel == 2) {
      UInt32 c16 = _toRGB565(ClearColor);

      return (c16 << 16) | c16;
    } else {
      // 24bpp (and any other byte-packed format): every byte of the
      // pixel triple is 0x10 since R == G == B in ClearColor, so a
      // dword of 0x10101010 fills cleanly regardless of byte alignment
      return 0x10101010;
    }
  }

  VESADriver::VESADriver() {
    auto* bootInfo = reinterpret_cast<Arch::IA32::Memory::E820BootInfo*>(0x8000);

    if (bootInfo->GraphicsFlags & 0x01) {
      _width = static_cast<UInt16>(bootInfo->GraphicsWidth);
      _height = static_cast<UInt16>(bootInfo->GraphicsHeight);
      _pitch = bootInfo->GraphicsPitch;
      _bpp = static_cast<UInt8>(bootInfo->GraphicsBPP);
      _fbPhysical = bootInfo->GraphicsFramebuffer;
      _fbSize = bootInfo->GraphicsFramebufferSize;
      _textColumns = _width / 8;
      _textRows = _height / FontGlyphHeight;
    }
  }

  void VESADriver::SetDependencies(
    IMaydayHandler* maydayHandler,
    IMemoryAllocator* memoryAllocator,
    SharedBufferRepository* sharedBuffers
  ) {
    _maydayHandler = maydayHandler;
    _memoryAllocator = memoryAllocator;
    _sharedBuffers = sharedBuffers;
  }

  void VESADriver::MapFramebuffer(
    IMemoryMapper* mapper,
    IAddressSpace* addressSpace
  ) {
    if (!_fbPhysical || !_fbSize || !mapper || !addressSpace) return;
    if (_bpp != 32 && _bpp != 24 && _bpp != 16) return;

    _bytesPerPixel = _bpp / 8;

    mapper->Map(
      *addressSpace,
      MemoryBlock {
        FramebufferAddress,
        _fbSize
      },
      MemoryBlock {
        _fbPhysical,
        _fbSize
      },
      Memory::MemoryMappingFlags {
        Memory::MemoryMappingPermissions::Read |
        Memory::MemoryMappingPermissions::Write,
        Memory::MemoryMappingCache::WriteCombining,
        Memory::MemoryMappingOptions::None
      }
    );

    _framebuffer = reinterpret_cast<volatile UInt8*>(FramebufferAddress);
    _shadow = new UInt8[_fbSize];

    if (!_shadow) MAYDAY("VESA: failed to allocate shadow framebuffer");

    // clear both shadow and framebuffer to ClearColor in the current
    // pixel format
    UInt32 totalDwords = (static_cast<UInt32>(_height) * _pitch) / 4;
    UInt32 fillPattern = _clearPattern();

    void* shadowDst = _shadow;
    UInt32 shadowCnt = totalDwords;

    asm volatile(
      "cld\n"
      "rep stosl"
      : "+D"(shadowDst), "+c"(shadowCnt)
      : "a"(fillPattern)
      : "memory"
    );

    void* clearDst = const_cast<UInt8*>(_framebuffer);
    UInt32 clearCnt = totalDwords;

    asm volatile(
      "cld\n"
      "rep stosl"
      : "+D"(clearDst), "+c"(clearCnt)
      : "a"(fillPattern)
      : "memory"
    );
  }

  // -------------------------------------------------------------------------
  // Text rendering
  // -------------------------------------------------------------------------

  void VESADriver::WriteCharacter(char character) {
    if (!_framebuffer) return;

    _lock.Acquire();

    _hideCursor();

    switch (character) {
      case '\n': {
        _cursorCol = 0;
        _cursorRow++;

        break;
      } case '\r': {
        _cursorCol = 0;

        break;
      } case '\b': {
        if (_cursorCol > 0) {
          _cursorCol--;
          _renderGlyph(_cursorCol, _cursorRow, ' ');
        } else if (_cursorRow > 0) {
          _cursorRow--;
          _cursorCol = _textColumns - 1;
          _renderGlyph(_cursorCol, _cursorRow, ' ');
        }

        break;
      } default: {
        _renderGlyph(_cursorCol, _cursorRow, character);
        _cursorCol++;

        if (_cursorCol >= _textColumns) {
          _cursorCol = 0;
          _cursorRow++;
        }

        break;
      }
    }

    if (_cursorRow >= _textRows) _scrollUp();

    _drawCursor();

    _lock.Release();
  }

  void VESADriver::WriteText(const char* string) {
    while (*string) WriteCharacter(*string++);
  }

  void VESADriver::SetTextForegroundColor(UInt32 color) {
    _textFg = color;
  }

  Geometry2D::Point VESADriver::GetTextCursorPosition() {
    return {
      static_cast<Int16>(_cursorCol),
      static_cast<Int16>(_cursorRow)
    };
  }

  void VESADriver::SetTextCursorPosition(Geometry2D::Point position) {
    _lock.Acquire();

    if (position.X < _textColumns) {
      _cursorCol = static_cast<UInt16>(position.X);
    }

    if (position.Y < _textRows) {
      _cursorRow = static_cast<UInt16>(position.Y);
    }

    _lock.Release();
  }

  // -------------------------------------------------------------------------
  // Graphics operations
  // -------------------------------------------------------------------------

  void VESADriver::SetMode(UInt16) {
    // VESA mode was configured by the bootloader. Nothing to do.
  }

  void VESADriver::FillRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    if (!_framebuffer) return;

    _lock.Acquire();

    UInt16 x2 = x + w;
    UInt16 y2 = y + h;

    if (x2 > _width) x2 = _width;
    if (y2 > _height) y2 = _height;
    if (x >= _width || y >= _height) { _lock.Release(); return; }

    UInt16 fillWidth = x2 - x;

    bool batch = _batchMode;

    if (_bytesPerPixel == 4) {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 4;

        // fill shadow (cached RAM, fast)
        {
          void* dst = _shadow + offset;
          UInt32 cnt = fillWidth;

          asm volatile(
            "cld\n"
            "rep stosl"
            : "+D"(dst), "+c"(cnt)
            : "a"(color)
            : "memory"
          );
        }

        // bulk copy shadow to framebuffer (skip in batch mode)
        if (!batch) {
          void* dst = const_cast<UInt8*>(_framebuffer + offset);
          void* src = _shadow + offset;
          UInt32 cnt = fillWidth;

          asm volatile(
            "cld\n"
            "rep movsl"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else if (_bytesPerPixel == 2) {
      UInt16 c16 = _toRGB565(color);

      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 2;

        // fill shadow (cached RAM, fast)
        {
          void* dst = _shadow + offset;
          UInt32 cnt = fillWidth;

          asm volatile(
            "cld\n"
            "rep stosw"
            : "+D"(dst), "+c"(cnt)
            : "a"(c16)
            : "memory"
          );
        }

        // bulk copy shadow to framebuffer (skip in batch mode)
        if (!batch) {
          void* dst = const_cast<UInt8*>(_framebuffer + offset);
          void* src = _shadow + offset;
          UInt32 cnt = fillWidth;

          asm volatile(
            "cld\n"
            "rep movsw"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 rowOffset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * _bytesPerPixel;

        // fill shadow (cached)
        for (UInt16 col = x; col < x2; ++col) {
          UInt32 off = static_cast<UInt32>(row) * _pitch
            + static_cast<UInt32>(col) * _bytesPerPixel;

          _shadow[off]     = color & 0xFF;
          _shadow[off + 1] = (color >> 8) & 0xFF;
          _shadow[off + 2] = (color >> 16) & 0xFF;
        }

        // bulk copy row to framebuffer (skip in batch mode)
        if (!batch) {
          UInt32 copyBytes = static_cast<UInt32>(fillWidth) * _bytesPerPixel;
          void* dst = const_cast<UInt8*>(_framebuffer + rowOffset);
          void* src = _shadow + rowOffset;
          UInt32 cnt = copyBytes;

          asm volatile(
            "cld\n"
            "rep movsb"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    }

    _lock.Release();
  }

  void VESADriver::BlitBuffer(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 transparentColor,
    const UInt32* pixels
  ) {
    if (!_framebuffer || !pixels) return;

    _lock.Acquire();

    UInt16 endX = x + w;

    if (endX > _width) endX = _width;

    bool batch = _batchMode;

    if (_bytesPerPixel == 4) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _height) break;

        UInt32 srcOffset = static_cast<UInt32>(row) * w;
        UInt16 copyWidth = endX - x;

        if (copyWidth == 0) continue;

        UInt32 fbRowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 4;

        // write to shadow (cached RAM, fast)
        UInt32* shadowRow
          = reinterpret_cast<UInt32*>(_shadow + fbRowOffset);

        for (UInt16 col = 0; col < copyWidth; ++col) {
          UInt32 pixel = pixels[srcOffset + col];

          if (pixel != transparentColor) {
            shadowRow[col] = pixel;
          }
        }

        // bulk copy shadow row to framebuffer (skip in batch mode)
        if (!batch) {
          void* dst = const_cast<UInt8*>(_framebuffer + fbRowOffset);
          void* src = _shadow + fbRowOffset;
          UInt32 cnt = copyWidth;

          asm volatile(
            "cld\n"
            "rep movsl"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else if (_bytesPerPixel == 2) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _height) break;

        UInt32 srcOffset = static_cast<UInt32>(row) * w;
        UInt16 copyWidth = endX - x;

        if (copyWidth == 0) continue;

        UInt32 fbRowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 2;

        // write to shadow (cached RAM, fast)
        UInt16* shadowRow
          = reinterpret_cast<UInt16*>(_shadow + fbRowOffset);

        for (UInt16 col = 0; col < copyWidth; ++col) {
          UInt32 pixel = pixels[srcOffset + col];

          if (pixel != transparentColor) {
            shadowRow[col] = _toRGB565(pixel);
          }
        }

        // bulk copy shadow row to framebuffer (skip in batch mode)
        if (!batch) {
          void* dst = const_cast<UInt8*>(_framebuffer + fbRowOffset);
          void* src = _shadow + fbRowOffset;
          UInt32 cnt = copyWidth;

          asm volatile(
            "cld\n"
            "rep movsw"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _height) break;

        UInt32 srcOffset = static_cast<UInt32>(row) * w;
        UInt16 copyWidth = endX - x;

        if (copyWidth == 0) continue;

        UInt32 fbRowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * _bytesPerPixel;

        // write to shadow (cached)
        for (UInt16 col = 0; col < copyWidth; ++col) {
          UInt32 pixel = pixels[srcOffset + col];

          if (pixel != transparentColor) {
            UInt32 off = static_cast<UInt32>(screenY) * _pitch
              + static_cast<UInt32>(x + col) * _bytesPerPixel;

            _shadow[off]     = pixel & 0xFF;
            _shadow[off + 1] = (pixel >> 8) & 0xFF;
            _shadow[off + 2] = (pixel >> 16) & 0xFF;
          }
        }

        // bulk copy row to framebuffer (skip in batch mode)
        if (!batch) {
          UInt32 copyBytes
            = static_cast<UInt32>(copyWidth) * _bytesPerPixel;
          void* dst = const_cast<UInt8*>(_framebuffer + fbRowOffset);
          void* src = _shadow + fbRowOffset;
          UInt32 cnt = copyBytes;

          asm volatile(
            "cld\n"
            "rep movsb"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    }

    _lock.Release();
  }

  void VESADriver::XORRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    if (!_framebuffer) return;

    _lock.Acquire();

    UInt16 x2 = x + w;
    UInt16 y2 = y + h;

    if (x2 > _width) x2 = _width;
    if (y2 > _height) y2 = _height;
    if (x >= _width || y >= _height) { _lock.Release(); return; }

    UInt16 xorWidth = x2 - x;
    bool batch = _batchMode;

    if (_bytesPerPixel == 4) {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 4;

        // XOR in shadow (cached reads + writes, no UC MMIO reads)
        UInt32* shadowRow
          = reinterpret_cast<UInt32*>(_shadow + offset);

        for (UInt16 col = 0; col < xorWidth; ++col) {
          shadowRow[col] ^= color;
        }

        // bulk copy to framebuffer (skip in batch mode)
        if (!batch) {
          void* dst = const_cast<UInt8*>(_framebuffer + offset);
          void* src = _shadow + offset;
          UInt32 cnt = xorWidth;

          asm volatile(
            "cld\n"
            "rep movsl"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else if (_bytesPerPixel == 2) {
      UInt16 c16 = _toRGB565(color);

      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 2;

        // XOR in shadow (cached reads + writes, no UC MMIO reads)
        UInt16* shadowRow
          = reinterpret_cast<UInt16*>(_shadow + offset);

        for (UInt16 col = 0; col < xorWidth; ++col) {
          shadowRow[col] ^= c16;
        }

        // bulk copy to framebuffer (skip in batch mode)
        if (!batch) {
          void* dst = const_cast<UInt8*>(_framebuffer + offset);
          void* src = _shadow + offset;
          UInt32 cnt = xorWidth;

          asm volatile(
            "cld\n"
            "rep movsw"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    } else {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 rowOffset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * _bytesPerPixel;

        // XOR in shadow (cached)
        for (UInt16 col = x; col < x2; ++col) {
          UInt32 off = static_cast<UInt32>(row) * _pitch
            + static_cast<UInt32>(col) * _bytesPerPixel;

          _shadow[off]     ^= color & 0xFF;
          _shadow[off + 1] ^= (color >> 8) & 0xFF;
          _shadow[off + 2] ^= (color >> 16) & 0xFF;
        }

        // bulk copy row to framebuffer (skip in batch mode)
        if (!batch) {
          UInt32 copyBytes
            = static_cast<UInt32>(xorWidth) * _bytesPerPixel;
          void* dst = const_cast<UInt8*>(_framebuffer + rowOffset);
          void* src = _shadow + rowOffset;
          UInt32 cnt = copyBytes;

          asm volatile(
            "cld\n"
            "rep movsb"
            : "+D"(dst), "+S"(src), "+c"(cnt)
            :
            : "memory"
          );
        }
      }
    }

    _lock.Release();
  }

  // -------------------------------------------------------------------------
  // Mode info and batch control
  // -------------------------------------------------------------------------

  void VESADriver::GetModeInfo(ModeInfoPayload* info) {
    if (!info) return;

    info->Width = _width;
    info->Height = _height;
    info->BitsPerPixel = _bpp;
    info->Pitch = static_cast<UInt16>(_pitch);
  }

  void VESADriver::SetBatchMode(bool enabled) {
    _lock.Acquire();
    _batchMode = enabled;
    _lock.Release();
  }

  void VESADriver::FlushRegion(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h
  ) {
    if (!_framebuffer || !_shadow) return;

    _lock.Acquire();

    UInt16 x2 = x + w;
    UInt16 y2 = y + h;

    if (x2 > _width) x2 = _width;
    if (y2 > _height) y2 = _height;
    if (x >= _width || y >= _height) { _lock.Release(); return; }

    UInt16 flushWidth = x2 - x;

    if (_bytesPerPixel == 4) {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 4;

        void* dst = const_cast<UInt8*>(_framebuffer + offset);
        void* src = _shadow + offset;
        UInt32 cnt = flushWidth;

        asm volatile(
          "cld\n"
          "rep movsl"
          : "+D"(dst), "+S"(src), "+c"(cnt)
          :
          : "memory"
        );
      }
    } else if (_bytesPerPixel == 2) {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 2;

        void* dst = const_cast<UInt8*>(_framebuffer + offset);
        void* src = _shadow + offset;
        UInt32 cnt = flushWidth;

        asm volatile(
          "cld\n"
          "rep movsw"
          : "+D"(dst), "+S"(src), "+c"(cnt)
          :
          : "memory"
        );
      }
    } else {
      for (UInt16 row = y; row < y2; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * _bytesPerPixel;

        UInt32 copyBytes = static_cast<UInt32>(flushWidth) * _bytesPerPixel;
        void* dst = const_cast<UInt8*>(_framebuffer + offset);
        void* src = _shadow + offset;
        UInt32 cnt = copyBytes;

        asm volatile(
          "cld\n"
          "rep movsb"
          : "+D"(dst), "+S"(src), "+c"(cnt)
          :
          : "memory"
        );
      }
    }

    _lock.Release();
  }

  void VESADriver::ScreenBlit(
    UInt16 srcX,
    UInt16 srcY,
    UInt16 dstX,
    UInt16 dstY,
    UInt16 w,
    UInt16 h
  ) {
    if (!_framebuffer || !_shadow || w == 0 || h == 0) return;

    if (srcX >= _width || srcY >= _height) return;
    if (dstX >= _width || dstY >= _height) return;
    if (srcX + w > _width) w = _width - srcX;
    if (srcY + h > _height) h = _height - srcY;
    if (dstX + w > _width) w = _width - dstX;
    if (dstY + h > _height) h = _height - dstY;

    if (w == 0 || h == 0) return;

    _lock.Acquire();

    bool reverseRow = (srcY == dstY && dstX > srcX);
    bool batch = _batchMode;

    if (dstY > srcY) {
      // bottom-to-top row iteration
      for (Int16 row = static_cast<Int16>(h - 1); row >= 0; --row) {
        UInt32 srcOff = static_cast<UInt32>(srcY + row) * _pitch
          + static_cast<UInt32>(srcX) * _bytesPerPixel;
        UInt32 dstOff = static_cast<UInt32>(dstY + row) * _pitch
          + static_cast<UInt32>(dstX) * _bytesPerPixel;
        UInt32 bytes = static_cast<UInt32>(w) * _bytesPerPixel;

        // rows at different Y, no within-row overlap, forward copy safe
        {
          const void* s = _shadow + srcOff;
          void* d = _shadow + dstOff;
          UInt32 dwords = bytes / 4;
          UInt32 tail = bytes % 4;

          if (dwords) {
            asm volatile(
              "cld\n"
              "rep movsl"
              : "+S"(s), "+D"(d), "+c"(dwords)
              :
              : "memory"
            );
          }

          if (tail) {
            const UInt8* sb = static_cast<const UInt8*>(s);
            UInt8* db = static_cast<UInt8*>(d);

            for (UInt32 i = 0; i < tail; ++i) {
              db[i] = sb[i];
            }
          }
        }

        // copy shadow row to framebuffer
        if (!batch) {
          void* dst = const_cast<UInt8*>(
            reinterpret_cast<volatile UInt8*>(_framebuffer + dstOff)
          );
          void* src = _shadow + dstOff;
          UInt32 dwords = bytes / 4;
          UInt32 tail = bytes % 4;

          if (dwords) {
            asm volatile(
              "cld\n"
              "rep movsl"
              : "+S"(src), "+D"(dst), "+c"(dwords)
              :
              : "memory"
            );
          }

          if (tail) {
            const UInt8* sb = static_cast<const UInt8*>(src);
            volatile UInt8* db = static_cast<volatile UInt8*>(
              reinterpret_cast<volatile void*>(dst)
            );

            for (UInt32 i = 0; i < tail; ++i) {
              db[i] = sb[i];
            }
          }
        }
      }
    } else {
      // top-to-bottom row iteration (also used when srcY == dstY)
      for (UInt16 row = 0; row < h; ++row) {
        UInt32 srcOff = static_cast<UInt32>(srcY + row) * _pitch
          + static_cast<UInt32>(srcX) * _bytesPerPixel;
        UInt32 dstOff = static_cast<UInt32>(dstY + row) * _pitch
          + static_cast<UInt32>(dstX) * _bytesPerPixel;
        UInt32 bytes = static_cast<UInt32>(w) * _bytesPerPixel;

        if (reverseRow) {
          // within-row overlap: copy bytes right-to-left
          for (Int32 i = static_cast<Int32>(bytes) - 1; i >= 0; --i) {
            _shadow[dstOff + i] = _shadow[srcOff + i];
          }
        } else {
          const void* s = _shadow + srcOff;
          void* d = _shadow + dstOff;
          UInt32 dwords = bytes / 4;
          UInt32 tail = bytes % 4;

          if (dwords) {
            asm volatile(
              "cld\n"
              "rep movsl"
              : "+S"(s), "+D"(d), "+c"(dwords)
              :
              : "memory"
            );
          }

          if (tail) {
            const UInt8* sb = static_cast<const UInt8*>(s);
            UInt8* db = static_cast<UInt8*>(d);

            for (UInt32 i = 0; i < tail; ++i) {
              db[i] = sb[i];
            }
          }
        }

        // copy shadow row to framebuffer
        if (!batch) {
          void* dst = const_cast<UInt8*>(
            reinterpret_cast<volatile UInt8*>(_framebuffer + dstOff)
          );
          void* src = _shadow + dstOff;
          UInt32 dwords = bytes / 4;
          UInt32 tail = bytes % 4;

          if (dwords) {
            asm volatile(
              "cld\n"
              "rep movsl"
              : "+S"(src), "+D"(dst), "+c"(dwords)
              :
              : "memory"
            );
          }

          if (tail) {
            const UInt8* sb = static_cast<const UInt8*>(src);
            volatile UInt8* db = static_cast<volatile UInt8*>(
              reinterpret_cast<volatile void*>(dst)
            );

            for (UInt32 i = 0; i < tail; ++i) {
              db[i] = sb[i];
            }
          }
        }
      }
    }

    _lock.Release();
  }

  void VESADriver::WritePixelRegion(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 srcPitch,
    const UInt32* pixels
  ) {
    if (!_framebuffer || !_shadow || !pixels) return;
    if (w == 0 || h == 0) return;
    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    if (_bytesPerPixel == 4) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        const UInt32* srcRow = &pixels[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];
        UInt32* shadowRow = reinterpret_cast<UInt32*>(
          _shadow + static_cast<UInt32>(screenY) * _pitch
            + static_cast<UInt32>(x) * 4
        );

        for (UInt16 col = 0; col < w; ++col) {
          shadowRow[col] = srcRow[col];
        }
      }
    } else if (_bytesPerPixel == 2) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        const UInt32* srcRow = &pixels[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];
        UInt16* shadowRow = reinterpret_cast<UInt16*>(
          _shadow + static_cast<UInt32>(screenY) * _pitch
            + static_cast<UInt32>(x) * 2
        );

        for (UInt16 col = 0; col < w; ++col) {
          shadowRow[col] = _toRGB565(srcRow[col]);
        }
      }
    } else {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        const UInt32* srcRow = &pixels[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];
        UInt8* shadowRow = _shadow
          + static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * _bytesPerPixel;

        for (UInt16 col = 0; col < w; ++col) {
          UInt32 argb = srcRow[col];
          UInt32 off = static_cast<UInt32>(col) * _bytesPerPixel;

          shadowRow[off]     = static_cast<UInt8>(argb);
          shadowRow[off + 1] = static_cast<UInt8>(argb >> 8);
          shadowRow[off + 2] = static_cast<UInt8>(argb >> 16);
        }
      }
    }

    _lock.Release();
  }

  void VESADriver::WriteNativeRegion(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 srcPitch,
    const void* pixels
  ) {
    if (!_framebuffer || !_shadow || !pixels) return;
    if (w == 0 || h == 0) return;
    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    if (_bytesPerPixel == 2) {
      const UInt16* src16 = static_cast<const UInt16*>(pixels);

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        const UInt16* srcRow = &src16[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];
        UInt16* shadowRow = reinterpret_cast<UInt16*>(
          _shadow + static_cast<UInt32>(screenY) * _pitch
            + static_cast<UInt32>(x) * 2
        );

        for (UInt16 col = 0; col < w; ++col) {
          shadowRow[col] = srcRow[col];
        }
      }
    } else if (_bytesPerPixel == 4) {
      const UInt32* src32 = static_cast<const UInt32*>(pixels);

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        const UInt32* srcRow = &src32[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];
        UInt32* shadowRow = reinterpret_cast<UInt32*>(
          _shadow + static_cast<UInt32>(screenY) * _pitch
            + static_cast<UInt32>(x) * 4
        );

        for (UInt16 col = 0; col < w; ++col) {
          shadowRow[col] = srcRow[col];
        }
      }
    }

    _lock.Release();
  }

  void VESADriver::_renderGlyph(UInt16 col, UInt16 row, char ch) {
    UInt16 px = col * 8;
    UInt16 py = row * FontGlyphHeight;
    const UInt8* glyph = Font[static_cast<UInt8>(ch)];

    if (_bytesPerPixel == 4) {
      UInt32 rowStride = _pitch / 4;
      auto* fb32 = reinterpret_cast<volatile UInt32*>(_framebuffer);
      auto* sh32 = reinterpret_cast<UInt32*>(_shadow);

      for (UInt16 y = 0; y < FontGlyphHeight; ++y) {
        UInt8 bits = glyph[y];
        UInt32 rowOff = static_cast<UInt32>(py + y) * rowStride + px;
        volatile UInt32* fbRow = fb32 + rowOff;
        UInt32* shRow = sh32 + rowOff;

        for (UInt16 x = 0; x < 8; ++x) {
          UInt32 c = (bits & (0x80 >> x)) ? _textFg : _textBg;
          fbRow[x] = c;
          shRow[x] = c;
        }
      }
    } else if (_bytesPerPixel == 2) {
      UInt32 rowStride = _pitch / 2;
      auto* fb16 = reinterpret_cast<volatile UInt16*>(_framebuffer);
      auto* sh16 = reinterpret_cast<UInt16*>(_shadow);
      UInt16 fg16 = _toRGB565(_textFg);
      UInt16 bg16 = _toRGB565(_textBg);

      for (UInt16 y = 0; y < FontGlyphHeight; ++y) {
        UInt8 bits = glyph[y];
        UInt32 rowOff = static_cast<UInt32>(py + y) * rowStride + px;
        volatile UInt16* fbRow = fb16 + rowOff;
        UInt16* shRow = sh16 + rowOff;

        for (UInt16 x = 0; x < 8; ++x) {
          UInt16 c = (bits & (0x80 >> x)) ? fg16 : bg16;
          fbRow[x] = c;
          shRow[x] = c;
        }
      }
    } else {
      for (UInt16 y = 0; y < FontGlyphHeight; ++y) {
        UInt8 bits = glyph[y];
        UInt32 rowOffset = static_cast<UInt32>(py + y) * _pitch
          + static_cast<UInt32>(px) * 3;

        for (UInt16 x = 0; x < 8; ++x) {
          UInt32 color = (bits & (0x80 >> x)) ? _textFg : _textBg;
          UInt32 offset = rowOffset + static_cast<UInt32>(x) * 3;
          _framebuffer[offset]     = color & 0xFF;
          _framebuffer[offset + 1] = (color >> 8) & 0xFF;
          _framebuffer[offset + 2] = (color >> 16) & 0xFF;
          _shadow[offset]     = color & 0xFF;
          _shadow[offset + 1] = (color >> 8) & 0xFF;
          _shadow[offset + 2] = (color >> 16) & 0xFF;
        }
      }
    }
  }

  void VESADriver::_scrollUp() {
    UInt32 scrollBytes = FontGlyphHeight * _pitch;
    UInt32 totalDwords = (static_cast<UInt32>(_height) * _pitch) / 4;
    UInt32 copyDwords = (static_cast<UInt32>(_height - FontGlyphHeight) * _pitch) / 4;
    UInt32 clearDwords = scrollBytes / 4;

    // 1) scroll within shadow buffer (cached RAM, fast)
    void* shDst = _shadow;
    void* shSrc = _shadow + scrollBytes;
    UInt32 shCnt = copyDwords;

    asm volatile(
      "cld\n"
      "rep movsl"
      : "+D"(shDst), "+S"(shSrc), "+c"(shCnt)
      :
      : "memory"
    );

    // 2) clear last FontGlyphHeight rows in shadow to ClearColor
    void* shClearDst = _shadow + copyDwords * 4;
    UInt32 shClearCnt = clearDwords;
    UInt32 fillPattern = _clearPattern();

    asm volatile(
      "cld\n"
      "rep stosl"
      : "+D"(shClearDst), "+c"(shClearCnt)
      : "a"(fillPattern)
      : "memory"
    );

    // 3) copy entire shadow to framebuffer (write-only MMIO, no slow reads)
    void* fbDst = const_cast<UInt8*>(_framebuffer);
    void* fbSrc = _shadow;
    UInt32 fbCnt = totalDwords;

    asm volatile(
      "cld\n"
      "rep movsl"
      : "+D"(fbDst), "+S"(fbSrc), "+c"(fbCnt)
      :
      : "memory"
    );

    _cursorRow = _textRows - 1;
    _cursorCol = 0;
  }

  void VESADriver::_drawCursor() {
    _cursorChar = ' ';

    UInt16 px = _cursorCol * 8;
    UInt16 py = _cursorRow * FontGlyphHeight;

    // the cursor covers the glyph area minus the top 1px and bottom
    // 1px of line spacing so it aligns with the visible character bounds
    constexpr UInt16 cursorHeight = FontGlyphHeight - 2;

    if (_bytesPerPixel == 4) {
      UInt32 rowStride = _pitch / 4;
      auto* fb32 = reinterpret_cast<volatile UInt32*>(_framebuffer);
      auto* sh32 = reinterpret_cast<UInt32*>(_shadow);

      for (UInt16 y = 0; y < cursorHeight; ++y) {
        UInt32 rowOff = static_cast<UInt32>(py + y) * rowStride + px;
        volatile UInt32* fbRow = fb32 + rowOff;
        UInt32* shRow = sh32 + rowOff;

        for (UInt16 x = 0; x < 8; ++x) {
          fbRow[x] = _textFg;
          shRow[x] = _textFg;
        }
      }
    } else if (_bytesPerPixel == 2) {
      UInt32 rowStride = _pitch / 2;
      auto* fb16 = reinterpret_cast<volatile UInt16*>(_framebuffer);
      auto* sh16 = reinterpret_cast<UInt16*>(_shadow);
      UInt16 fg16 = _toRGB565(_textFg);

      for (UInt16 y = 0; y < cursorHeight; ++y) {
        UInt32 rowOff = static_cast<UInt32>(py + y) * rowStride + px;
        volatile UInt16* fbRow = fb16 + rowOff;
        UInt16* shRow = sh16 + rowOff;

        for (UInt16 x = 0; x < 8; ++x) {
          fbRow[x] = fg16;
          shRow[x] = fg16;
        }
      }
    } else {
      for (UInt16 y = 0; y < cursorHeight; ++y) {
        for (UInt16 x = 0; x < 8; ++x) {
          _putPixel(px + x, py + y, _textFg);
        }
      }
    }

    _cursorDrawn = true;
  }

  void VESADriver::_hideCursor() {
    if (!_cursorDrawn) return;

    _renderGlyph(_cursorCol, _cursorRow, _cursorChar);
    _cursorDrawn = false;
  }

  void VESADriver::_putPixel(UInt16 x, UInt16 y, UInt32 color) {
    UInt32 offset = static_cast<UInt32>(y) * _pitch
      + static_cast<UInt32>(x) * _bytesPerPixel;

    if (_bytesPerPixel == 4) {
      *reinterpret_cast<volatile UInt32*>(_framebuffer + offset) = color;

      if (_shadow) {
        *reinterpret_cast<UInt32*>(_shadow + offset) = color;
      }
    } else if (_bytesPerPixel == 2) {
      UInt16 c16 = _toRGB565(color);
      *reinterpret_cast<volatile UInt16*>(_framebuffer + offset) = c16;

      if (_shadow) {
        *reinterpret_cast<UInt16*>(_shadow + offset) = c16;
      }
    } else {
      _framebuffer[offset]     = color & 0xFF;
      _framebuffer[offset + 1] = (color >> 8) & 0xFF;
      _framebuffer[offset + 2] = (color >> 16) & 0xFF;

      if (_shadow) {
        _shadow[offset]     = color & 0xFF;
        _shadow[offset + 1] = (color >> 8) & 0xFF;
        _shadow[offset + 2] = (color >> 16) & 0xFF;
      }
    }
  }

  void VESADriver::GetFramebufferBufferID(
    FramebufferBufferIDPayload* out
  ) {
    if (!out) return;

    if (_framebufferBufferID == 0 && _fbPhysical && _fbSize
        && _sharedBuffers) {
      Size blockSize = _memoryAllocator->GetBlockSize();
      Size blockCount = (_fbSize + blockSize - 1) / blockSize;
      auto* blocks = new MemoryBlock[blockCount];

      for (Size i = 0; i < blockCount; i++) {
        blocks[i] = MemoryBlock {
          _fbPhysical + i * blockSize,
          blockSize
        };
      }

      Memory::SharedBuffer* buf = _sharedBuffers->Create(
        blocks, blockCount, _fbSize
      );
      buf->Pinned = true;
      buf->CacheMode = Memory::MemoryMappingCache::WriteCombining;
      _framebufferBufferID = buf->ID;

      KLOG_INFO(
        "VRAM SharedBuffer ID %u (%u blocks)",
        _framebufferBufferID, blockCount
      );
    }

    out->BufferID = _framebufferBufferID;
  }

  UInt32 VESADriver::_getPixel(UInt16 x, UInt16 y) {
    UInt32 offset = static_cast<UInt32>(y) * _pitch
      + static_cast<UInt32>(x) * _bytesPerPixel;

    if (_bytesPerPixel == 4) {
      return *reinterpret_cast<volatile UInt32*>(_framebuffer + offset);
    } else if (_bytesPerPixel == 2) {
      UInt16 c16 = *reinterpret_cast<volatile UInt16*>(_framebuffer + offset);
      return ((c16 & 0xF800) << 8)
        | ((c16 & 0x07E0) << 5)
        | ((c16 & 0x001F) << 3)
        | 0xFF000000;
    } else {
      return static_cast<UInt32>(_framebuffer[offset])
        | (static_cast<UInt32>(_framebuffer[offset + 1]) << 8)
        | (static_cast<UInt32>(_framebuffer[offset + 2]) << 16)
        | 0xFF000000;
    }
  }

  bool VESADriver::AcquireDisplay(UInt32 ownerPID) {
    if (ownerPID == 0) return false;

    _lock.Acquire();

    if (_displayOwnerPID != 0) {
      _lock.Release();

      return false;
    }

    _displayOwnerPID = ownerPID;

    _lock.Release();

    return true;
  }

  void VESADriver::ReleaseDisplay(UInt32 ownerPID) {
    _lock.Acquire();

    if (_displayOwnerPID == ownerPID) {
      _displayOwnerPID = 0;
    }

    _lock.Release();
  }

  void VESADriver::GetDisplayOwner(DisplayOwnerPayload* out) {
    if (!out) return;

    _lock.Acquire();

    out->OwnerPID = _displayOwnerPID;
    out->IsCompositing = (_displayOwnerPID != 0);

    _lock.Release();
  }
}
