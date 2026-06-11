/**
 * @file Kernel/Platform/PC/Drivers/Graphics/S3ViRGEDriver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Graphics::S3ViRGEDriver.
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

#include "S3ViRGEDriver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Graphics {
  using namespace Arch;

  bool S3ViRGEDriver::Discover(Bus::PCI* pci) {
    if (!pci) return false;

    for (UInt16 id : DeviceIDs) {
      Bus::PCIDeviceInfo pciInfo = {};

      if (pci->FindDevice(S3VendorID, id, &pciInfo)) {
        return true;
      }
    }

    return false;
  }

  S3ViRGEDriver::S3ViRGEDriver(
    Bus::PCI* pci,
    ICPUDriver* cpu,
    IMemoryAllocator* memoryAllocator,
    SharedBufferRepository* sharedBuffers
  ) :
    _pci(pci),
    _cpu(cpu),
    _memoryAllocator(memoryAllocator),
    _sharedBuffers(sharedBuffers)
  {
    Bus::PCIDeviceInfo pciInfo = {};

    for (UInt16 id : DeviceIDs) {
      if (_pci->FindDevice(S3VendorID, id, &pciInfo)) {
        _found = true;
        _bar0 = pciInfo.BAR[0] & 0xFFFFFFF0;

        SetPCIAddress(_device, {
          pciInfo.Bus,
          pciInfo.Slot,
          pciInfo.Function
        });

        break;
      }
    }

    if (!_found) return;

    auto* bootInfo = reinterpret_cast<Arch::IA32::Memory::E820BootInfo*>(0x8000);

    if (bootInfo->GraphicsFlags & 0x01) {
      _width = static_cast<UInt16>(bootInfo->GraphicsWidth);
      _height = static_cast<UInt16>(bootInfo->GraphicsHeight);
      _pitch = bootInfo->GraphicsPitch;
      _bpp = static_cast<UInt8>(bootInfo->GraphicsBPP);
    }
  }

  bool S3ViRGEDriver::Initialize(
    IMemoryMapper* mapper,
    IAddressSpace* addressSpace
  ) {
    if (!_found || !mapper || !addressSpace) return false;
    if (_bpp != 16 && _bpp != 24 && _bpp != 32) return false;

    _bytesPerPixel = _bpp / 8;

    // map the MMIO register space (BAR0 + 16 MB, 64 KB)
    // the page tables stay mapped permanently; CR53 controls whether
    // the S3 chip responds to those addresses
    UInt32 mmioPhysical = _bar0 + MMIOOffset;

    mapper->Map(
      *addressSpace,
      MemoryBlock {
        MMIOVirtualAddress,
        MMIOSize
      },
      MemoryBlock {
        mmioPhysical,
        MMIOSize
      },
      MemoryMappingFlags {
        MemoryMappingPermissions::Read |
        MemoryMappingPermissions::Write,
        MemoryMappingCache::Uncached,
        MemoryMappingOptions::None
      }
    );

    _mmio = reinterpret_cast<volatile UInt8*>(MMIOVirtualAddress);
    _framebuffer = reinterpret_cast<volatile UInt8*>(FramebufferAddress);

    // unlock S3 extended registers
    _unlockRegisters();

    // save the base CR53 value (with MMIO disabled)
    _cpu->Out8(0x3D4, 0x53);

    _cr53Base = _cpu->In8(0x3D5) & ~0x08;

    // enable MMIO permanently, the BLT engine needs it to stay on
    _enableMMIO();

    UInt32 status = _mmioRead32(RegisterSubsystemStatus);
    PCIAddress pciAddress = GetPCIAddress(_device);

    KLOG_TRACE(
      "Discovered S3 ViRGE at PCI %u:%u.%u (BAR0 0x%x, MMIO 0x%x, status 0x%x)",
      pciAddress.Bus,
      pciAddress.Slot,
      pciAddress.Function,
      _bar0,
      mmioPhysical,
      status
    );

    // disable the hardware cursor
    _cpu->Out8(0x3D4, 0x45);
    _cpu->In8(0x3D5); // reading CR45 resets the cursor color stack
    _cpu->Out8(0x3D4, 0x45);
    _cpu->Out8(0x3D5, 0x00); // clear bit 0 = cursor off

    // move cursor position off-screen
    _cpu->Out8(0x3D4, 0x46);
    _cpu->Out8(0x3D5, 0x07);
    _cpu->Out8(0x3D4, 0x47);
    _cpu->Out8(0x3D5, 0xFF);
    _cpu->Out8(0x3D4, 0x48);
    _cpu->Out8(0x3D5, 0x07);
    _cpu->Out8(0x3D4, 0x49);
    _cpu->Out8(0x3D5, 0xFF);

    // allocate shadow buffer and copy existing LFB content into it
    // (preserves boot text rendered by VESA before S3 ViRGE took over)
    UInt32 fbSize = static_cast<UInt32>(_height) * _pitch;

    _shadow = new UInt8[fbSize];

    // fast copy from VRAM to shadow using rep movsl; the shadow is uncached RAM
    // so this is much faster than reading from VRAM via the CPU, especially at
    // 16 bpp where the CPU reads 32 bits at a time but only 16 bits are valid,
    // causing extra bus transactions and halving effective throughput
    {
      const void* source = const_cast<const UInt8*>(
        reinterpret_cast<volatile UInt8*>(_framebuffer)
      );
      void* destination = _shadow;
      UInt32 count = fbSize / 4;

      asm volatile(
        "cld\n"
        "rep movsl"
        : "+S"(source), "+D"(destination), "+c"(count)
        :
        : "memory"
      );
    }

    _textColumns = _width / 8;
    _textRows = _height / FontGlyphHeight;

    // hardware cursor setup
    // allocate 1 KB of VRAM immediately after the visible framebuffer (1 KB
    // aligned)
    // cursor data: 512-byte AND plane + 512-byte XOR plane
    // AND=1,XOR=0 -> transparent; AND=0,XOR=0 -> black; AND=0,XOR=1 -> white
    _cursorVramBase
      = (static_cast<UInt32>(_height) * _pitch
        + CursorVramAlignment - 1)
      & ~static_cast<UInt32>(CursorVramAlignment - 1);

    // Map VRAM pages covering the cursor slot. VESA maps exactly
    // height×pitch bytes; for resolutions where that ends on a page
    // boundary (e.g. 640×480@16bpp -> 0x96000) the cursor slot falls on
    // the first unmapped page, causing a page fault on write.
    // Map() is a no-op for pages already covered (Phase 1.2 protection).
    {
      UInt32 cursorEnd = _cursorVramBase + CursorDataSize;
      UInt32 pageStart = _cursorVramBase & ~static_cast<UInt32>(0xFFF);
      UInt32 pageEnd   = (cursorEnd + 0xFFF) & ~static_cast<UInt32>(0xFFF);
      UInt32 pageSize  = pageEnd - pageStart;

      mapper->Map(
        *addressSpace,
        MemoryBlock {
          FramebufferAddress + pageStart,
          pageSize
        },
        MemoryBlock {
          _bar0 + pageStart,
          pageSize
        },
        MemoryMappingFlags {
          MemoryMappingPermissions::Read |
          MemoryMappingPermissions::Write,
          MemoryMappingCache::WriteCombining,
          MemoryMappingOptions::None
        }
      );
    }

    // write blank cursor (all transparent) to VRAM
    // S3 ViRGE format: 64 rows × 16 bytes per row.
    // Within each row, AND and XOR are interleaved in 2-byte pairs for
    // each 16-pixel band: [AND_0,AND_1, XOR_0,XOR_1, AND_2,AND_3, XOR_2,XOR_3, ...]
    // Transparent = AND=0xFF, XOR=0x00.
    {
      volatile UInt8* dest = _framebuffer + _cursorVramBase;

      for (UInt32 i = 0; i < CursorDataSize; i += 4) {
        dest[i]     = 0xFF;  // AND (transparent)
        dest[i + 1] = 0xFF;
        dest[i + 2] = 0x00;  // XOR (no flip)
        dest[i + 3] = 0x00;
      }
    }

    // Set cursor VRAM address: CR4C = high byte, CR4D = low byte of
    // (cursorVramBase / 1024).
    {
      UInt32 unit = _cursorVramBase >> 10;

      _cpu->Out8(0x3D4, 0x4C);
      _cpu->Out8(0x3D5, static_cast<UInt8>(unit >> 8));
      _cpu->Out8(0x3D4, 0x4D);
      _cpu->Out8(0x3D5, static_cast<UInt8>(unit & 0xFF));
    }

    // Set cursor colors via the S3 color stack.
    // Reading CR45 resets the 3-byte write pointer (must precede each color).
    // AND=0,XOR=1 pixels use CR4A; AND=0,XOR=0 pixels use CR4B.
    // Our cursor: W (white outline) -> AND=0,XOR=1 -> CR4A = white
    //             K (black body)    -> AND=0,XOR=0 -> CR4B = black
    _cpu->Out8(0x3D4, 0x45);
    _cpu->In8(0x3D5);                // reset color-stack pointer

    _cpu->Out8(0x3D4, 0x4A);        // CR4A (AND=0,XOR=1 pixels) = white
    _cpu->Out8(0x3D5, 0xFF);        // R
    _cpu->Out8(0x3D5, 0xFF);        // G
    _cpu->Out8(0x3D5, 0xFF);        // B

    _cpu->Out8(0x3D4, 0x45);
    _cpu->In8(0x3D5);                // reset color-stack pointer (required before CR4B)

    _cpu->Out8(0x3D4, 0x4B);        // CR4B (AND=0,XOR=0 pixels) = black
    _cpu->Out8(0x3D5, 0x00);        // R
    _cpu->Out8(0x3D5, 0x00);        // G
    _cpu->Out8(0x3D5, 0x00);        // B

    // Cursor remains disabled (CR45 = 0x00, already written above)

    _device.State = DeviceState::Active;
    _initialized = true;

    return true;
  }

  void S3ViRGEDriver::WriteCharacter(char character) {
    _writeCharacter(character);
  }

  void S3ViRGEDriver::WriteText(const char* text) {
    while (*text) _writeCharacter(*text++);
  }

  void S3ViRGEDriver::SetTextForegroundColor(UInt32 color) {
    _textFg = color;
  }

  Geometry2D::Point S3ViRGEDriver::GetTextCursorPosition() {
    return {
      static_cast<Int16>(_cursorCol),
      static_cast<Int16>(_cursorRow)
    };
  }

  void S3ViRGEDriver::SetTextCursorPosition(Geometry2D::Point position) {
    if (position.X < _textColumns) _cursorCol = static_cast<UInt16>(position.X);
    if (position.Y < _textRows) _cursorRow = static_cast<UInt16>(position.Y);
  }

  void S3ViRGEDriver::SetMode(UInt16) {
    // TODO
  }

  void S3ViRGEDriver::FillRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    if (!_initialized || w == 0 || h == 0) return;

    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    // always update shadow (cached RAM, fast)
    if (_bytesPerPixel == 2) {
      UInt16 c16 = _toRGB565(color);

      for (UInt16 row = y; row < y + h; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 2;
        void* dst = _shadow + offset;
        UInt32 cnt = w;

        asm volatile(
          "cld\n"
          "rep stosw"
          : "+D"(dst), "+c"(cnt)
          : "a"(c16)
          : "memory"
        );
      }
    }

    // In batch mode the shadow is the target; FlushRegion will copy it
    // to VRAM atomically, preventing intermediate states from being
    // visible. Outside batch mode, use the GPU BLT for speed.
    if (_batchMode) { _hasDeferredWrites = true; _lock.Release(); return; }

    {
      UInt32 fmtBits;
      UInt32 fgColor;

      if (_bpp == 16) {
        fmtBits = Format16Bpp;
        fgColor = _toRGB565(color);
      } else {
        fmtBits = Format24Bpp;
        fgColor = color & 0x00FFFFFF;
      }

      // Queue one BLT packet without stalling for full engine idle.
      _waitFifo(6);

      _mmioWrite32(RegisterDestinationBase, 0);
      _mmioWrite32(RegisterDestinationSourceStride, (_pitch << 16) | _pitch);
      _mmioWrite32(RegisterPatternForegroundColor, fgColor);
      _mmioWrite32(
        RegisterRectangleWidthHeight,
        (static_cast<UInt32>(w - 1) << 16) | static_cast<UInt32>(h)
      );
      _mmioWrite32(
        RegisterDestinationXY,
        (static_cast<UInt32>(x) << 16) | static_cast<UInt32>(y)
      );
      _mmioWrite32(
        RegisterCommandSet,
        CommandRectFill | RopPatternCopy | CommandXPositive
          | CommandYPositive | CommandDisableBlockWrite | CommandDraw | fmtBits
      );
      (void)_mmioRead32(RegisterSubsystemStatus);
    }

    _lock.Release();
  }

  void S3ViRGEDriver::BlitBuffer(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 transparentColor,
    const UInt32* pixels
  ) {
    if (!_initialized || !pixels || w == 0 || h == 0) return;

    _lock.Acquire();

    // wait for any pending BLT to finish before touching the LFB
    _waitIdle();

    UInt16 endX = x + w;
    UInt16 endY = y + h;

    if (endX > _width) endX = _width;
    if (endY > _height) endY = _height;

    if (_batchMode) _hasDeferredWrites = true;

    if (_bytesPerPixel == 2) {
      UInt16 copyWidth = endX - x;

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _height) break;

        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 2;
        UInt16* shadowRow = reinterpret_cast<UInt16*>(_shadow + rowOffset);
        UInt32 srcOffset = static_cast<UInt32>(row) * w;

        if (_batchMode) {
          // shadow only, FlushRegion will copy to VRAM later
          for (UInt16 col = 0; col < copyWidth; ++col) {
            UInt32 pixel = pixels[srcOffset + col];

            if (pixel != transparentColor) {
              shadowRow[col] = _toRGB565(pixel);
            }
          }
        } else {
          // update both shadow and VRAM
          volatile UInt16* fbRow
            = reinterpret_cast<volatile UInt16*>(_framebuffer + rowOffset);

          for (UInt16 col = 0; col < copyWidth; ++col) {
            UInt32 pixel = pixels[srcOffset + col];

            if (pixel != transparentColor) {
              UInt16 c16 = _toRGB565(pixel);

              shadowRow[col] = c16;
              fbRow[col] = c16;
            }
          }
        }
      }
    } else if (_bytesPerPixel == 4) {
      UInt16 copyWidth = endX - x;

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _height) break;

        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 4;
        UInt32* shadowRow = reinterpret_cast<UInt32*>(_shadow + rowOffset);
        UInt32 srcOffset = static_cast<UInt32>(row) * w;

        if (_batchMode) {
          for (UInt16 col = 0; col < copyWidth; ++col) {
            UInt32 pixel = pixels[srcOffset + col];

            if (pixel != transparentColor) {
              shadowRow[col] = pixel;
            }
          }
        } else {
          volatile UInt32* fbRow
            = reinterpret_cast<volatile UInt32*>(_framebuffer + rowOffset);

          for (UInt16 col = 0; col < copyWidth; ++col) {
            UInt32 pixel = pixels[srcOffset + col];

            if (pixel != transparentColor) {
              shadowRow[col] = pixel;
              fbRow[col] = pixel;
            }
          }
        }
      }
    } else {
      UInt16 copyWidth = endX - x;

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;

        if (screenY >= _height) break;

        UInt32 srcOffset = static_cast<UInt32>(row) * w;

        for (UInt16 col = 0; col < copyWidth; ++col) {
          UInt32 pixel = pixels[srcOffset + col];

          if (pixel != transparentColor) {
            UInt32 off = static_cast<UInt32>(screenY) * _pitch
              + static_cast<UInt32>(x + col) * _bytesPerPixel;

            _shadow[off] = pixel & 0xFF;
            _shadow[off + 1] = (pixel >> 8) & 0xFF;
            _shadow[off + 2] = (pixel >> 16) & 0xFF;

            if (!_batchMode) {
              _framebuffer[off] = pixel & 0xFF;
              _framebuffer[off + 1] = (pixel >> 8) & 0xFF;
              _framebuffer[off + 2] = (pixel >> 16) & 0xFF;
            }
          }
        }
      }
    }

    _lock.Release();
  }

  void S3ViRGEDriver::XORRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    if (!_initialized || w == 0 || h == 0) return;

    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    // always XOR the shadow buffer (cached RAM, fast read-modify-write)
    if (_bytesPerPixel == 2) {
      UInt16 c16 = _toRGB565(color);

      for (UInt16 row = y; row < y + h; ++row) {
        UInt32 offset
          = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 2;
        UInt16* shadowRow = reinterpret_cast<UInt16*>(_shadow + offset);

        for (UInt16 col = 0; col < w; ++col) {
          shadowRow[col] ^= c16;
        }
      }
    }

    // always BLT XOR to VRAM, single atomic GPU operation, no flicker,
    // and avoids expensive FlushRegion CPU copies during drag moves
    {
      UInt32 fmtBits;
      UInt32 fgColor;

      if (_bpp == 16) {
        fmtBits = Format16Bpp;
        fgColor = _toRGB565(color);
      } else {
        fmtBits = Format24Bpp;
        fgColor = color & 0x00FFFFFF;
      }

      // Queue one BLT packet without stalling for full engine idle.
      _waitFifo(6);

      _mmioWrite32(RegisterDestinationBase, 0);
      _mmioWrite32(RegisterDestinationSourceStride, (_pitch << 16) | _pitch);
      _mmioWrite32(RegisterPatternForegroundColor, fgColor);
      _mmioWrite32(
        RegisterRectangleWidthHeight,
        (static_cast<UInt32>(w - 1) << 16) | static_cast<UInt32>(h)
      );
      _mmioWrite32(
        RegisterDestinationXY,
        (static_cast<UInt32>(x) << 16) | static_cast<UInt32>(y)
      );
      _mmioWrite32(
        RegisterCommandSet,
        CommandRectFill |
        RopPatternXor |
        CommandXPositive |
        CommandYPositive |
        CommandDisableBlockWrite |
        CommandDraw |
        fmtBits
      );
      (void)_mmioRead32(RegisterSubsystemStatus);
    }

    _lock.Release();
  }

  void S3ViRGEDriver::GetModeInfo(ModeInfoPayload* info) {
    if (!info) return;

    info->Width = _width;
    info->Height = _height;
    info->BitsPerPixel = _bpp;
    info->Pitch = static_cast<UInt16>(_pitch);
  }

  void S3ViRGEDriver::SetBatchMode(bool enabled) {
    _batchMode = enabled;

    if (enabled) _hasDeferredWrites = false;
  }

  void S3ViRGEDriver::FlushRegion(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h
  ) {
    if (!_initialized || !_shadow || w == 0 || h == 0) return;
    if (!_hasDeferredWrites) return;

    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    // wait for any pending BLT before writing to the LFB
    _waitIdle();

    if (_bytesPerPixel == 2) {
      for (UInt16 row = y; row < y + h; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * 2;
        const void* src = _shadow + offset;
        void* dst = const_cast<UInt8*>(
          reinterpret_cast<volatile UInt8*>(_framebuffer + offset)
        );
        UInt32 dwords = w / 2;
        UInt32 tailWords = w & 1;

        if (dwords) {
          asm volatile(
            "cld\n"
            "rep movsl"
            : "+S"(src), "+D"(dst), "+c"(dwords)
            :
            : "memory"
          );
        }

        if (tailWords) {
          *static_cast<volatile UInt16*>(
            reinterpret_cast<volatile void*>(dst)
          ) = *static_cast<const UInt16*>(src);
        }
      }
    } else {
      UInt32 bytesPerRow = static_cast<UInt32>(w) * _bytesPerPixel;

      for (UInt16 row = y; row < y + h; ++row) {
        UInt32 offset = static_cast<UInt32>(row) * _pitch
          + static_cast<UInt32>(x) * _bytesPerPixel;

        const void* src = _shadow + offset;
        void* dst = const_cast<UInt8*>(
          reinterpret_cast<volatile UInt8*>(_framebuffer + offset)
        );

        UInt32 dwords = bytesPerRow / 4;
        UInt32 tail = bytesPerRow % 4;

        if (dwords) {
          const void* s = src;
          void* d = dst;

          asm volatile(
            "cld\n"
            "rep movsl"
            : "+S"(s), "+D"(d), "+c"(dwords)
            :
            : "memory"
          );

          src = s;
          dst = d;
        }

        if (tail) {
          const UInt8* sb = static_cast<const UInt8*>(src);
          volatile UInt8* db = static_cast<volatile UInt8*>(
            reinterpret_cast<volatile void*>(dst)
          );

          for (UInt32 i = 0; i < tail; ++i) db[i] = sb[i];
        }
      }
    }

    _lock.Release();
  }

  void S3ViRGEDriver::WritePixelRegion(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 srcPitch,
    const UInt32* pixels
  ) {
    if (!_initialized || !_shadow || w == 0 || h == 0) return;
    if (!pixels) return;

    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    // When not in batch mode, write to both shadow and VRAM in a single
    // pass, this eliminates the separate FlushRegion copy and uses DWORD
    // PCI writes (2 pixels per bus transaction at 16bpp), roughly doubling
    // throughput vs the old rep-movsw FlushRegion.
    bool directFlush = !_batchMode;

    if (directFlush) _waitIdle();

    if (_bytesPerPixel == 2) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 2;
        const UInt32* srcRow = &pixels[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];

        if (directFlush) {
          volatile UInt32* fbRow32 = reinterpret_cast<volatile UInt32*>(
            _framebuffer + rowOffset
          );
          UInt16 col = 0;

          for (; col + 1 < w; col += 2) {
            UInt16 c0 = _toRGB565(srcRow[col]);
            UInt16 c1 = _toRGB565(srcRow[col + 1]);

            fbRow32[col >> 1] = static_cast<UInt32>(c0)
              | (static_cast<UInt32>(c1) << 16);
          }

          if (col < w) {
            reinterpret_cast<volatile UInt16*>(
              _framebuffer + rowOffset
            )[col] = _toRGB565(srcRow[col]);
          }
        } else {
          UInt16* shadowRow = reinterpret_cast<UInt16*>(
            _shadow + rowOffset
          );
          UInt16 col = 0;

          for (; col + 1 < w; col += 2) {
            UInt16 c0 = _toRGB565(srcRow[col]);
            UInt16 c1 = _toRGB565(srcRow[col + 1]);

            *reinterpret_cast<UInt32*>(&shadowRow[col])
              = static_cast<UInt32>(c0) | (static_cast<UInt32>(c1) << 16);
          }

          if (col < w) shadowRow[col] = _toRGB565(srcRow[col]);
        }
      }
    } else if (_bytesPerPixel == 3) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 3;
        const UInt32* srcRow = &pixels[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];

        if (directFlush) {
          volatile UInt8* fbRow = _framebuffer + rowOffset;

          for (UInt16 col = 0; col < w; ++col) {
            UInt32 argb = srcRow[col];
            UInt32 off = static_cast<UInt32>(col) * 3;

            fbRow[off]     = static_cast<UInt8>(argb);
            fbRow[off + 1] = static_cast<UInt8>(argb >> 8);
            fbRow[off + 2] = static_cast<UInt8>(argb >> 16);
          }
        } else {
          UInt8* shadowRow = _shadow + rowOffset;

          for (UInt16 col = 0; col < w; ++col) {
            UInt32 argb = srcRow[col];
            UInt32 off = static_cast<UInt32>(col) * 3;

            shadowRow[off]     = static_cast<UInt8>(argb);
            shadowRow[off + 1] = static_cast<UInt8>(argb >> 8);
            shadowRow[off + 2] = static_cast<UInt8>(argb >> 16);
          }
        }
      }
    } else if (_bytesPerPixel == 4) {
      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 4;
        const UInt32* srcRow = &pixels[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];

        if (directFlush) {
          volatile UInt32* fbRow = reinterpret_cast<volatile UInt32*>(
            _framebuffer + rowOffset
          );

          for (UInt16 col = 0; col < w; ++col) {
            fbRow[col] = srcRow[col];
          }
        } else {
          UInt32* shadowRow = reinterpret_cast<UInt32*>(
            _shadow + rowOffset
          );

          for (UInt16 col = 0; col < w; ++col) {
            shadowRow[col] = srcRow[col];
          }
        }
      }
    }

    _hasDeferredWrites = !directFlush;

    _lock.Release();
  }

  void S3ViRGEDriver::WriteNativeRegion(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 srcPitch,
    const void* pixels
  ) {
    if (!_initialized || !_shadow || w == 0 || h == 0) return;
    if (!pixels) return;
    if (x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;

    _lock.Acquire();

    bool directFlush = !_batchMode;

    if (directFlush) _waitIdle();

    if (_bytesPerPixel == 2) {
      const UInt16* src16 = static_cast<const UInt16*>(pixels);

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 2;
        const UInt16* srcRow = &src16[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];

        if (directFlush) {
          volatile UInt32* fbRow32 = reinterpret_cast<volatile UInt32*>(
            _framebuffer + rowOffset
          );
          UInt16 col = 0;

          for (; col + 1 < w; col += 2) {
            fbRow32[col >> 1] = static_cast<UInt32>(srcRow[col])
              | (static_cast<UInt32>(srcRow[col + 1]) << 16);
          }

          if (col < w) {
            reinterpret_cast<volatile UInt16*>(
              _framebuffer + rowOffset
            )[col] = srcRow[col];
          }
        } else {
          UInt16* shadowRow = reinterpret_cast<UInt16*>(
            _shadow + rowOffset
          );

          for (UInt16 col = 0; col < w; ++col) {
            shadowRow[col] = srcRow[col];
          }
        }
      }
    } else if (_bytesPerPixel == 4) {
      const UInt32* src32 = static_cast<const UInt32*>(pixels);

      for (UInt16 row = 0; row < h; ++row) {
        UInt16 screenY = y + row;
        UInt32 rowOffset = static_cast<UInt32>(screenY) * _pitch
          + static_cast<UInt32>(x) * 4;
        const UInt32* srcRow = &src32[
          static_cast<UInt32>(screenY) * srcPitch + x
        ];

        if (directFlush) {
          volatile UInt32* fbRow = reinterpret_cast<volatile UInt32*>(
            _framebuffer + rowOffset
          );

          for (UInt16 col = 0; col < w; ++col) {
            fbRow[col] = srcRow[col];
          }
        } else {
          UInt32* shadowRow = reinterpret_cast<UInt32*>(
            _shadow + rowOffset
          );

          for (UInt16 col = 0; col < w; ++col) {
            shadowRow[col] = srcRow[col];
          }
        }
      }
    }

    _hasDeferredWrites = !directFlush;

    _lock.Release();
  }

  void S3ViRGEDriver::ScreenBlit(
    UInt16 srcX,
    UInt16 srcY,
    UInt16 dstX,
    UInt16 dstY,
    UInt16 w,
    UInt16 h
  ) {
    if (!_initialized || w == 0 || h == 0) return;

    if (srcX >= _width || srcY >= _height) return;
    if (dstX >= _width || dstY >= _height) return;
    if (srcX + w > _width) w = _width - srcX;
    if (srcY + h > _height) h = _height - srcY;
    if (dstX + w > _width) w = _width - dstX;
    if (dstY + h > _height) h = _height - dstY;

    if (w == 0 || h == 0) return;

    _lock.Acquire();

    // GPU BITBLT SRCCOPY, VRAM-to-VRAM only, no shadow memmove.
    // The AppServer composite buffer is the authoritative readable copy;
    // the shadow is allowed to be stale here because nothing in the
    // FlushBackBuffer pipeline reads it after a ScreenBlit.
    {
      UInt32 cmdBits = 0;
      UInt16 bltSrcX, bltSrcY, bltDstX, bltDstY;

      // XP=1: left-to-right scan (use left edge coords)
      // XP=0: right-to-left scan (use right edge coords)
      if (dstX <= srcX) {
        cmdBits |= CommandXPositive;
        bltSrcX = srcX;
        bltDstX = dstX;
      } else {
        bltSrcX = srcX + w - 1;
        bltDstX = dstX + w - 1;
      }

      // YP=1: top-to-bottom scan (use top edge coords)
      // YP=0: bottom-to-top scan (use bottom edge coords)
      if (dstY <= srcY) {
        cmdBits |= CommandYPositive;
        bltSrcY = srcY;
        bltDstY = dstY;
      } else {
        bltSrcY = srcY + h - 1;
        bltDstY = dstY + h - 1;
      }

      UInt32 fmtBits = (_bpp == 16) ? Format16Bpp : Format24Bpp;

      // Ensure FIFO has room for 7 register writes. _waitFifo falls back to
      // _waitIdle if the FIFO slot count is unavailable, so this also
      // serialises against any prior in-flight BLT.
      _waitFifo(7);

      _mmioWrite32(RegisterSourceBase, 0);
      _mmioWrite32(RegisterDestinationBase, 0);
      _mmioWrite32(RegisterDestinationSourceStride, (_pitch << 16) | _pitch);
      _mmioWrite32(
        RegisterSourceXY,
        (static_cast<UInt32>(bltSrcX) << 16) | static_cast<UInt32>(bltSrcY)
      );
      _mmioWrite32(
        RegisterDestinationXY,
        (static_cast<UInt32>(bltDstX) << 16) | static_cast<UInt32>(bltDstY)
      );
      _mmioWrite32(
        RegisterRectangleWidthHeight,
        (static_cast<UInt32>(w - 1) << 16) | static_cast<UInt32>(h)
      );
      _mmioWrite32(
        RegisterCommandSet,
        CommandBitBlt |
        RopSourceCopy |
        cmdBits |
        CommandDisableBlockWrite |
        CommandDraw |
        fmtBits
      );

      // The BLT engine executes asynchronously on 86Box's FIFO thread.
      // We must wait for completion before returning, because the caller
      // (AppServer) immediately composites exposed strips into the same
      // VRAM the BLT is reading from.
      _waitIdle();
    }

    _lock.Release();
  }

  void S3ViRGEDriver::_mmioWrite32(UInt32 offset, UInt32 value) {
    *reinterpret_cast<volatile UInt32*>(_mmio + offset) = value;
  }

  UInt32 S3ViRGEDriver::_mmioRead32(UInt32 offset) {
    return *reinterpret_cast<volatile UInt32*>(_mmio + offset);
  }

  void S3ViRGEDriver::_enableMMIO() {
    _cpu->Out8(0x3D4, 0x53);
    _cpu->Out8(0x3D5, _cr53Base | 0x08);
  }

  void S3ViRGEDriver::_disableMMIO() {
    _cpu->Out8(0x3D4, 0x53);
    _cpu->Out8(0x3D5, _cr53Base);
  }

  void S3ViRGEDriver::_waitIdle() {
    // Spin until SUBSYS_STATUS bit 13 is SET (all engines idle).
    // On 86Box, MMIO register writes are queued to a FIFO processed by
    // a separate host thread, BLTs do NOT execute synchronously within
    // the CMD_SET write. This wait is required after any BLT to ensure
    // completion before the CPU accesses overlapping VRAM.
    //
    // Bounded spin to prevent permanent hang on hardware error.
    UInt32 timeout = 1u << 22; // ~4M iterations

    while (!(_mmioRead32(RegisterSubsystemStatus) & StatusEngineIdle)) {
      if (--timeout == 0) return;
    }
  }

  void S3ViRGEDriver::_waitFifo(UInt8 entries) {
    if (entries == 0) return;
    if (entries > 0x1F) entries = 0x1F;

    // ViRGE reports free command FIFO slots in SUBSYS_STATUS[12:8].
    // If the field is unavailable on a variant, fall back to idle wait.
    UInt32 spin = 1u << 20;

    while (spin--) {
      UInt32 status = _mmioRead32(RegisterSubsystemStatus);
      UInt32 freeEntries = (status & StatusFifoFreeMask) >> StatusFifoFreeShift;

      if (freeEntries >= entries) return;
    }

    _waitIdle();
  }

  void S3ViRGEDriver::_unlockRegisters() {
    // unlock S3 sequencer extensions: SR08 = 0x06
    _cpu->Out8(0x3C4, 0x08);
    _cpu->Out8(0x3C5, 0x06);

    // unlock S3 CRTC registers: CR38 = 0x48, CR39 = 0xA5
    _cpu->Out8(0x3D4, 0x38);
    _cpu->Out8(0x3D5, 0x48);
    _cpu->Out8(0x3D4, 0x39);
    _cpu->Out8(0x3D5, 0xA5);
  }

  void S3ViRGEDriver::_writeCharacter(char ch) {
    if (!_framebuffer) return;

    _lock.Acquire();

    _hideTextCursor();

    switch (ch) {
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
        _renderGlyph(_cursorCol, _cursorRow, ch);

        _cursorCol++;

        if (_cursorCol >= _textColumns) {
          _cursorCol = 0;
          _cursorRow++;
        }

        break;
      }
    }

    if (_cursorRow >= _textRows) _scrollUp();

    _drawTextCursor();

    _lock.Release();
  }

  void S3ViRGEDriver::_renderGlyph(UInt16 col, UInt16 row, char ch) {
    UInt16 px = col * 8;
    UInt16 py = row * FontGlyphHeight;
    const UInt8* glyph = Font[static_cast<UInt8>(ch)];

    if (_bytesPerPixel == 2) {
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
    } else if (_bytesPerPixel == 4) {
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
    } else {
      for (UInt16 y = 0; y < FontGlyphHeight; ++y) {
        UInt8 bits = glyph[y];
        UInt32 rowOffset
          = static_cast<UInt32>(py + y) * _pitch
          + static_cast<UInt32>(px) * 3;

        for (UInt16 x = 0; x < 8; ++x) {
          UInt32 color = (bits & (0x80 >> x)) ? _textFg : _textBg;
          UInt32 offset = rowOffset + static_cast<UInt32>(x) * 3;

          _shadow[offset] = color & 0xFF;
          _shadow[offset + 1] = (color >> 8) & 0xFF;
          _shadow[offset + 2] = (color >> 16) & 0xFF;
          _framebuffer[offset] = color & 0xFF;
          _framebuffer[offset + 1] = (color >> 8) & 0xFF;
          _framebuffer[offset + 2] = (color >> 16) & 0xFF;
        }
      }
    }
  }

  UInt32 S3ViRGEDriver::_clearPattern() const {
    if (_bytesPerPixel == 4) {
      return ClearColor;
    } else if (_bytesPerPixel == 2) {
      UInt32 c16 = _toRGB565(ClearColor);

      return (c16 << 16) | c16;
    } else {
      // 24bpp: every byte of the pixel triple is 0x10 since R == G == B
      // in ClearColor, so a dword of 0x10101010 fills cleanly regardless
      // of byte alignment
      return 0x10101010;
    }
  }

  void S3ViRGEDriver::_scrollUp() {
    UInt32 scrollBytes = FontGlyphHeight * _pitch;
    UInt32 totalDwords = (static_cast<UInt32>(_height) * _pitch) / 4;
    UInt32 copyDwords = (static_cast<UInt32>(_height - FontGlyphHeight) * _pitch) / 4;
    UInt32 clearDwords = scrollBytes / 4;

    // scroll within shadow buffer (cached RAM, fast)
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

    // clear last FontGlyphHeight rows in shadow to ClearColor
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

    // copy entire shadow to framebuffer (write-only MMIO, no slow reads)
    void* fbDst = const_cast<UInt8*>(
      reinterpret_cast<volatile UInt8*>(_framebuffer)
    );
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

  void S3ViRGEDriver::_drawTextCursor() {
    _cursorChar = ' ';

    UInt16 px = _cursorCol * 8;
    UInt16 py = _cursorRow * FontGlyphHeight;

    if (_bytesPerPixel == 2) {
      UInt32 rowStride = _pitch / 2;
      auto* fb16 = reinterpret_cast<volatile UInt16*>(_framebuffer);
      auto* sh16 = reinterpret_cast<UInt16*>(_shadow);
      UInt16 fg16 = _toRGB565(_textFg);

      for (UInt16 y = 0; y < FontGlyphHeight - 2; ++y) {
        UInt32 rowOff = static_cast<UInt32>(py + y) * rowStride + px;
        volatile UInt16* fbRow = fb16 + rowOff;
        UInt16* shRow = sh16 + rowOff;

        for (UInt16 x = 0; x < 8; ++x) {
          fbRow[x] = fg16;
          shRow[x] = fg16;
        }
      }
    } else if (_bytesPerPixel == 4) {
      UInt32 rowStride = _pitch / 4;
      auto* fb32 = reinterpret_cast<volatile UInt32*>(_framebuffer);
      auto* sh32 = reinterpret_cast<UInt32*>(_shadow);

      for (UInt16 y = 0; y < FontGlyphHeight - 2; ++y) {
        UInt32 rowOff = static_cast<UInt32>(py + y) * rowStride + px;
        volatile UInt32* fbRow = fb32 + rowOff;
        UInt32* shRow = sh32 + rowOff;

        for (UInt16 x = 0; x < 8; ++x) {
          fbRow[x] = _textFg;
          shRow[x] = _textFg;
        }
      }
    } else {
      for (UInt16 y = 0; y < FontGlyphHeight - 2; ++y) {
        UInt32 rowOffset
          = static_cast<UInt32>(py + y) * _pitch
          + static_cast<UInt32>(px) * 3;

        for (UInt16 x = 0; x < 8; ++x) {
          UInt32 offset = rowOffset + static_cast<UInt32>(x) * 3;

          _shadow[offset] = _textFg & 0xFF;
          _shadow[offset + 1] = (_textFg >> 8) & 0xFF;
          _shadow[offset + 2] = (_textFg >> 16) & 0xFF;
          _framebuffer[offset] = _textFg & 0xFF;
          _framebuffer[offset + 1] = (_textFg >> 8) & 0xFF;
          _framebuffer[offset + 2] = (_textFg >> 16) & 0xFF;
        }
      }
    }

    _cursorDrawn = true;
  }

  void S3ViRGEDriver::_hideTextCursor() {
    if (!_cursorDrawn) return;

    _renderGlyph(_cursorCol, _cursorRow, _cursorChar);

    _cursorDrawn = false;
  }

  void S3ViRGEDriver::GetHardwareCursorSupport(
    HardwareCursorSupportPayload* out
  ) {
    if (out) out->Supported = _initialized;
  }

  void S3ViRGEDriver::GetFramebufferBufferID(
    FramebufferBufferIDPayload* out
  ) {
    if (!out) return;

    if (
      _framebufferBufferID == 0 &&
      _initialized &&
      _bar0 &&
      _sharedBuffers
    ) {
      UInt32 framebufferSize = static_cast<UInt32>(_height) * _pitch;
      Size blockSize = _memoryAllocator->GetBlockSize();
      Size blockCount = (framebufferSize + blockSize - 1) / blockSize;
      MemoryBlock* blocks = new MemoryBlock[blockCount];

      for (
        Size blockIndex = 0;
        blockIndex < blockCount;
        blockIndex++
      ) {
        blocks[blockIndex] = MemoryBlock {
          _bar0 + blockIndex * blockSize,
          blockSize
        };
      }

      SharedBuffer* buffer = _sharedBuffers->Create(
        blocks,
        blockCount,
        framebufferSize
      );

      buffer->Pinned = true;
      buffer->CacheMode = MemoryMappingCache::WriteCombining;

      _framebufferBufferID = buffer->ID;

      KLOG_INFO(
        "VRAM SharedBuffer ID %u (%u blocks)",
        _framebufferBufferID,
        blockCount
      );
    }

    out->BufferID = _framebufferBufferID;
  }

  void S3ViRGEDriver::GetFastScreenBlitSupport(
    FastScreenBlitSupportPayload* out
  ) {
    if (out) out->Supported = _initialized;
  }

  void S3ViRGEDriver::SetHardwareCursorBitmap(
    UInt8 srcWidth,
    UInt8 srcHeight,
    UInt32 transparentColor,
    const UInt32* pixels
  ) {
    if (!_initialized || !pixels) return;

    _lock.Acquire();

    // no _waitIdle() needed: cursor VRAM is at _cursorVramBase (past the
    // visible framebuffer) and does not overlap with BLT engine targets

    // S3 ViRGE hardware cursor VRAM format: 64 rows × 16 bytes per row
    // each row is split into four 16-pixel bands (0-15, 16-31, 32-47, 48-63)
    // within each band: [AND_byte0, AND_byte1, XOR_byte0, XOR_byte1] (4 bytes)
    // bit 7 of each byte = leftmost pixel in that byte
    // AND=1,XOR=0 -> transparent; AND=0,XOR=0 -> CR4B (black);
    // AND=0,XOR=1 -> CR4A (white)
    // Initialise to all-transparent
    for (UInt32 i = 0; i < CursorDataSize; i += 4) {
      _cursorBuffer[i] = 0xFF;
      _cursorBuffer[i + 1] = 0xFF;
      _cursorBuffer[i + 2] = 0x00;
      _cursorBuffer[i + 3] = 0x00;
    }

    UInt8 maxCol = (srcWidth  < 64) ? srcWidth  : static_cast<UInt8>(64);
    UInt8 maxRow = (srcHeight < 64) ? srcHeight : static_cast<UInt8>(64);

    for (UInt8 row = 0; row < maxRow; ++row) {
      for (UInt8 col = 0; col < maxCol; ++col) {
        UInt32 pixel = pixels[static_cast<UInt32>(row) * srcWidth + col];

        if (pixel == transparentColor) continue;

        UInt8 alpha = static_cast<UInt8>((pixel >> 24) & 0xFF);

        // dither semi-transparent pixels: use a checkerboard pattern
        // scaled by alpha to approximate transparency
        if (alpha < 0xFF) {
          // skip pixel if alpha is below the dither threshold for
          // this checkerboard cell; 2x2 ordered dither matrix
          // thresholds: [64, 192, 192, 64]
          UInt8 threshold = ((row ^ col) & 1) ? 192 : 64;

          if (alpha < threshold) continue;
        }

        UInt8 band      = col >> 4;
        UInt8 posInBand = col & 15;
        UInt8 byteIdx   = posInBand >> 3;
        UInt8 bitMask   = static_cast<UInt8>(0x80 >> (posInBand & 7));

        UInt32 andOff = static_cast<UInt32>(row) * 16
          + static_cast<UInt32>(band) * 4
          + byteIdx;
        UInt32 xorOff = andOff + 2;

        _cursorBuffer[andOff] &= ~bitMask;

        if ((pixel & 0x00FFFFFF) != 0) {
          _cursorBuffer[xorOff] |= bitMask;
        }
      }
    }

    volatile UInt8* dest = _framebuffer + _cursorVramBase;

    for (UInt32 i = 0; i < CursorDataSize; ++i) dest[i] = _cursorBuffer[i];

    _lock.Release();
  }

  void S3ViRGEDriver::SetHardwareCursorPosition(Int16 x, Int16 y) {
    if (!_initialized) return;

    // Pixel offsets clip the cursor sprite when it hangs off the left/top
    // edge of the screen; must be even per S3 spec.
    UInt8 xOff = 0;
    UInt8 yOff = 0;

    if (x < 0) { xOff = static_cast<UInt8>((-x) & 0xFE); x = 0; }
    if (y < 0) { yOff = static_cast<UInt8>((-y) & 0xFE); y = 0; }

    UInt16 cx = static_cast<UInt16>(x);
    UInt16 cy = static_cast<UInt16>(y);

    _cpu->Out8(0x3D4, 0x4E); _cpu->Out8(0x3D5, xOff);  // CR4E: X pixel offset
    _cpu->Out8(0x3D4, 0x4F); _cpu->Out8(0x3D5, yOff);  // CR4F: Y pixel offset

    _cpu->Out8(0x3D4, 0x47); _cpu->Out8(0x3D5, static_cast<UInt8>(cx & 0xFF));
    _cpu->Out8(0x3D4, 0x46); _cpu->Out8(0x3D5, static_cast<UInt8>((cx >> 8) & 0x07));
    _cpu->Out8(0x3D4, 0x49); _cpu->Out8(0x3D5, static_cast<UInt8>(cy & 0xFF));
    _cpu->Out8(0x3D4, 0x48); _cpu->Out8(0x3D5, static_cast<UInt8>((cy >> 8) & 0x07)); // latch
  }

  void S3ViRGEDriver::SetHardwareCursorVisible(bool visible) {
    if (!_initialized) return;

    // Reading CR45 resets the S3 color-stack write pointer (required by spec
    // before any write to CR45, CR4A, or CR4B).
    _cpu->Out8(0x3D4, 0x45);
    _cpu->In8(0x3D5);

    _cpu->Out8(0x3D4, 0x45);
    _cpu->Out8(0x3D5, visible ? 0x01 : 0x00);
  }

  bool S3ViRGEDriver::AcquireDisplay(UInt32 ownerPID) {
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

  void S3ViRGEDriver::ReleaseDisplay(UInt32 ownerPID) {
    _lock.Acquire();

    if (_displayOwnerPID == ownerPID) {
      _displayOwnerPID = 0;
    }

    _lock.Release();
  }

  void S3ViRGEDriver::GetDisplayOwner(DisplayOwnerPayload* out) {
    if (!out) return;

    _lock.Acquire();

    out->OwnerPID = _displayOwnerPID;
    out->IsCompositing = (_displayOwnerPID != 0);

    _lock.Release();
  }
}
