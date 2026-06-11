/**
 * @file Bootloader/Platform/PC/PCPlatformInitializer.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::PCPlatformInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FAT12FileLoader.hpp"
#include "HAL.hpp"
#include "PCPlatformInitializer.hpp"
#include "VGASpinner.hpp"

namespace Quantum::Bootloader::Platform::PC {
  void PCPlatformInitializer::Initialize(
    PlatformInitializationOptions options
  ) {
    // load Boot's GDT (adds 16-bit segments for the BIOS trampoline)
    InitializeBIOS();

    x86BootInfo* info = static_cast<x86BootInfo*>(options.Info);

    // read boot drive from BootInfo (written by Stage2 before PM entry)
    UInt8 bootDrive = static_cast<UInt8>(info->Reserved & 0xFF);

    // re-store boot drive with 'BO' tag (kernel convention)
    info->Reserved = 0x424F0000 | bootDrive;

    static FAT12FileLoader fileLoader;
    static BIOSVGADriver vgaDriver;
    static BIOSKeyboardDriver keyboardDriver;
    static BIOSTimerDriver timerDriver;
    static VGASpinner spinner;

    fileLoader.Initialize(bootDrive);
    fileLoader.SetSpinner(&spinner);

    options.Context->FileLoader = &fileLoader;
    options.Context->Graphics = &vgaDriver;
    options.Context->Keyboard = &keyboardDriver;
    options.Context->Timer = &timerDriver;
    options.Context->KernelPhysicalAddress
      = KernelPhysicalAddress;
    options.Context->InitialImagePhysicalAddress
      = InitialImagePhysicalAddress;
    options.Context->TextColumns = BIOSVGADriver::Columns;
    options.Context->TextRows = BIOSVGADriver::Rows;
  }

  void PCPlatformInitializer::PrepareKernelHandoff(
    PlatformInitializationOptions options
  ) {
    x86BootInfo* info = static_cast<x86BootInfo*>(options.Info);

    info->InitialImagePhysicalAddress
      = options.Context->InitialImagePhysicalAddress;
    info->InitialImageSizeInBytes
      = options.Context->InitialImageSizeInBytes;

    // read the binary header at the start of the initial image:
    //   [0] = entry point offset from image base
    //   [1] = process binary image size
    UInt32* imageHeader = reinterpret_cast<UInt32*>(
      options.Context->InitialImagePhysicalAddress
    );
    info->InitialProcessEntryPointOffset = imageHeader[0];
    info->InitialProcessImageSize = imageHeader[1];
    info->InitialProcessAddress = InitialProcessAddress;

    // copy the initial process name from options
    const char* sourceName = options.Context->InitialProcessName;
    UInt32 nameIndex = 0;
    for (; nameIndex < 31 && sourceName[nameIndex] != '\0'; ++nameIndex) {
      info->InitialProcessName[nameIndex] = sourceName[nameIndex];
    }
    info->InitialProcessName[nameIndex] = '\0';

    _collectMemoryMap(options);
    _setupDisplay(options);
  }

  void PCPlatformInitializer::_setGraphicsFailed(IBootInfo* bootInfo) {
    x86BootInfo* info = static_cast<x86BootInfo*>(bootInfo);

    info->GraphicsFlags = 0;
    info->GraphicsWidth = 0;
    info->GraphicsHeight = 0;
    info->GraphicsPitch = 0;
    info->GraphicsBPP = 0;
    info->GraphicsFramebuffer = 0;
    info->GraphicsFramebufferSize = 0;
  }

  bool PCPlatformInitializer::_trySetVESAMode(UInt16 mode) {
    BIOSRegisters regs = {};

    regs.EAX = 0x4F02;
    regs.EBX = mode | 0x4000; // request linear framebuffer

    CallBIOS(0x10, &regs);

    return (regs.EAX & 0xFFFF) == 0x004F;
  }

  bool PCPlatformInitializer::_getVESAModeInfo(UInt16 mode) {
    BIOSRegisters regs = {};

    regs.EAX = 0x4F01;
    regs.ECX = mode;
    regs.ES = static_cast<UInt16>(VESAModeInfoPhysicalAddress >> 4);
    regs.EDI = VESAModeInfoPhysicalAddress & 0xF;

    CallBIOS(0x10, &regs);

    return (regs.EAX & 0xFFFF) == 0x004F;
  }

  void PCPlatformInitializer::_storeVESAInfo(IBootInfo* bootInfo) {
    x86BootInfo* info = static_cast<x86BootInfo*>(bootInfo);
    UInt8* mi = reinterpret_cast<UInt8*>(VESAModeInfoPhysicalAddress);
    UInt16 attrs = *reinterpret_cast<UInt16*>(mi + 0x00);

    if (!(attrs & 0x80)) {
      _setGraphicsFailed(info);

      return;
    }

    info->GraphicsFlags = 1 | 2; // has graphics + has LFB
    info->GraphicsWidth =
      *reinterpret_cast<UInt16*>(mi + 0x12);
    info->GraphicsHeight =
      *reinterpret_cast<UInt16*>(mi + 0x14);
    info->GraphicsPitch =
      *reinterpret_cast<UInt16*>(mi + 0x10);
    info->GraphicsBPP =
      *reinterpret_cast<UInt8*>(mi + 0x19);
    info->GraphicsFramebuffer =
      *reinterpret_cast<UInt32*>(mi + 0x28);
    info->GraphicsFramebufferSize =
      info->GraphicsPitch * info->GraphicsHeight;
  }

  void PCPlatformInitializer::_collectMemoryMap(
    PlatformInitializationOptions options
  ) {
    x86BootInfo* info = static_cast<x86BootInfo*>(options.Info);

    BIOSRegisters regs = {};
    UInt32 continuation = 0;
    UInt32 count = 0;

    constexpr UInt32 entriesBase
      = BootInfoPhysicalAddress
      + __builtin_offsetof(x86BootInfo, Entries);

    while (count < 32) {
      regs = {};
      regs.EAX = 0xE820;
      regs.EBX = continuation;
      regs.ECX = 20;
      regs.EDX = 0x534D4150; // 'SMAP'

      // point ES:DI at the next entry slot in BootInfo
      UInt32 entryAddr = entriesBase + count * 20;

      regs.ES = static_cast<UInt16>(entryAddr >> 4);
      regs.EDI = entryAddr & 0xF;

      CallBIOS(0x15, &regs);

      // CF set = error or end of list
      if (regs.EFLAGS & 1) break;

      // BIOS must echo 'SMAP' in EAX
      if (regs.EAX != 0x534D4150) break;

      count++;
      continuation = regs.EBX;

      if (continuation == 0) break;
    }

    info->EntryCount = count;
  }

  void PCPlatformInitializer::_setupDisplay(
    PlatformInitializationOptions options
  ) {
    // clear the text-mode screen
    BIOSRegisters regs = {};

    regs.EAX = 0x0003; // set 80x25 text mode (clears screen)

    CallBIOS(0x10, &regs);

    // enumerate VESA modes looking for 1024x768x16bpp with LFB
    regs = {};
    regs.EAX = 0x4F00;
    regs.ES = static_cast<UInt16>(VESAInfoPhysicalAddress >> 4);
    regs.EDI = VESAInfoPhysicalAddress & 0xF;

    CallBIOS(0x10, &regs);

    bool enumerated = (regs.EAX & 0xFFFF) == 0x004F;

    if (enumerated) {
      // read mode list far pointer (offset 0x0E in VESA info block)
      UInt16 modeListOfs =
        *reinterpret_cast<UInt16*>(VESAInfoPhysicalAddress + 0x0E);
      UInt16 modeListSeg =
        *reinterpret_cast<UInt16*>(VESAInfoPhysicalAddress + 0x10);
      UInt32 modeListAddr =
        (static_cast<UInt32>(modeListSeg) << 4) + modeListOfs;

      // walk mode list looking for 1024x768x16bpp with LFB
      while (true) {
        UInt16 mode = *reinterpret_cast<UInt16*>(modeListAddr);

        if (mode == 0xFFFF) break;

        modeListAddr += 2;

        if (!_getVESAModeInfo(mode)) continue;

        UInt8* mi = reinterpret_cast<UInt8*>(VESAModeInfoPhysicalAddress);
        UInt16 width = *reinterpret_cast<UInt16*>(mi + 0x12);
        UInt16 height = *reinterpret_cast<UInt16*>(mi + 0x14);
        UInt8 bpp = *reinterpret_cast<UInt8*>(mi + 0x19);
        UInt16 attrs = *reinterpret_cast<UInt16*>(mi + 0x00);

        if (width == 1024 && height == 768 && bpp == 16 && (attrs & 0x80)) {
          if (_trySetVESAMode(mode)) {
            _storeVESAInfo(options.Info);

            return;
          }
        }
      }
    }

    // fallback: try fixed mode 0x117 (1024x768x16bpp)
    if (_getVESAModeInfo(0x117) && _trySetVESAMode(0x117)) {
      _storeVESAInfo(options.Info);

      return;
    }

    _setGraphicsFailed(options.Info);
  }
}
