/**
 * @file System/Kernel/Arch/IA32/Bootstrap.cpp
 * @brief IA32 bootstrap code.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/BootInfo.hpp"
#include "Arch/IA32/Bootstrap.hpp"
#include "Arch/IA32/LinkerSymbols.hpp"
#include "Logger.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  [[gnu::section(".text.start.data")]]
  alignas(4096) UInt32 Bootstrap::_bootstrapPageDirectory[1024];

  [[gnu::section(".text.start.data")]]
  alignas(4096) UInt32 Bootstrap::_bootstrapPageTables[4][1024];

  [[gnu::section(".text.start.data")]]
  alignas(4096) UInt32 Bootstrap::_bootstrapKernelTables[8][1024];

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInitBundlePhys = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInitBundleSize = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInitBundleMagic0 = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInitBundleMagic1 = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInitBundlePayload0 = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInitBundlePayload1 = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInfoPhysical = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInfoEntryCount = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 Bootstrap::_bootInfoReserved = 0;

  [[gnu::section(".text.start")]]
  void Bootstrap::CaptureBootInfo(UInt32 bootInfoPhysicalAddress) {
    constexpr UInt32 payloadOffset = 0x2000;

    _bootInfoPhysical = bootInfoPhysicalAddress;

    if (bootInfoPhysicalAddress != 0) {
      const Arch::IA32::BootInfo::Raw* bootInfo
        = reinterpret_cast<const Arch::IA32::BootInfo::Raw*>(
          bootInfoPhysicalAddress
        );

      _bootInfoEntryCount = bootInfo->entryCount;
      _bootInfoReserved = bootInfo->reserved;
      _bootInitBundlePhys = bootInfo->initBundlePhysical;
      _bootInitBundleSize = bootInfo->initBundleSize;

      if (_bootInitBundlePhys != 0 && _bootInitBundleSize >= 8) {
        const UInt8* magicBase
          = reinterpret_cast<const UInt8*>(_bootInitBundlePhys);

        _bootInitBundleMagic0 = *reinterpret_cast<const UInt32*>(magicBase);
        _bootInitBundleMagic1 = *reinterpret_cast<const UInt32*>(magicBase + 4);
      }

      if (
        _bootInitBundlePhys != 0
        && _bootInitBundleSize >= payloadOffset + 8
      ) {
        const UInt8* payloadBase
          = reinterpret_cast<const UInt8*>(_bootInitBundlePhys + payloadOffset);

        _bootInitBundlePayload0
          = *reinterpret_cast<const UInt32*>(payloadBase);
        _bootInitBundlePayload1
          = *reinterpret_cast<const UInt32*>(payloadBase + 4);
      } else {
        _bootInitBundlePayload0 = 0;
        _bootInitBundlePayload1 = 0;
      }
    } else {
      _bootInfoEntryCount = 0;
      _bootInfoReserved = 0;
      _bootInitBundlePhys = 0;
      _bootInitBundleSize = 0;
      _bootInitBundleMagic0 = 0;
      _bootInitBundleMagic1 = 0;
      _bootInitBundlePayload0 = 0;
      _bootInitBundlePayload1 = 0;
    }
  }

  [[gnu::section(".text.start")]]
  void Bootstrap::BuildBootstrapPaging() {
    // clear directory
    for (UInt32 i = 0; i < 1024; ++i) {
      _bootstrapPageDirectory[i] = 0;
    }

    // identity map first 16 mb (4 tables)
    for (UInt32 t = 0; t < 4; ++t) {
      UInt32* table = _bootstrapPageTables[t];

      for (UInt32 entryIndex = 0; entryIndex < 1024; ++entryIndex) {
        UInt32 physicalAddress = (t * 1024 + entryIndex) * _pageSize;

        table[entryIndex] = physicalAddress | _pagePresent | _pageWrite;
      }

      _bootstrapPageDirectory[t]
        = reinterpret_cast<UInt32>(table)
        | _pagePresent
        | _pageWrite;
    }

    // map kernel higher-half: map the loaded higher-half image
    UInt32 kernelPhysicalStart = reinterpret_cast<UInt32>(&__hh_phys_start);
    UInt32 kernelPhysicalEnd = reinterpret_cast<UInt32>(&__phys_end);
    UInt32 kernelImageBytes = kernelPhysicalEnd - kernelPhysicalStart;
    UInt32 kernelVirtualBase = reinterpret_cast<UInt32>(&__hh_virt_start);
    UInt32 nextKernelTable = 0;

    // map each page of the kernel image
    for (UInt32 offset = 0; offset < kernelImageBytes; offset += _pageSize) {
      UInt32 physicalAddress = kernelPhysicalStart + offset;
      UInt32 virtualAddress = kernelVirtualBase + offset;
      UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
      UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

      if (_bootstrapPageDirectory[pageDirectoryIndex] == 0) {
        UInt32 tablePhysical = 0;

        if (pageDirectoryIndex < 4) {
          tablePhysical = reinterpret_cast<UInt32>(
            _bootstrapPageTables[pageDirectoryIndex]
          );
        } else if (nextKernelTable < 8) {
          UInt32* table = _bootstrapKernelTables[nextKernelTable++];

          for (UInt32 i = 0; i < 1024; ++i) {
            table[i] = 0;
          }

          tablePhysical = reinterpret_cast<UInt32>(table);
        }

        _bootstrapPageDirectory[pageDirectoryIndex]
          = tablePhysical | _pagePresent | _pageWrite;
      }

      UInt32* table = reinterpret_cast<UInt32*>(
        _bootstrapPageDirectory[pageDirectoryIndex] & ~0xFFFu
      );

      table[pageTableIndex] = physicalAddress | _pagePresent | _pageWrite;
    }

    // install recursive mapping
    _bootstrapPageDirectory[_recursiveSlot]
      = reinterpret_cast<UInt32>(_bootstrapPageDirectory)
      | _pagePresent
      | _pageWrite;
  }

  [[gnu::section(".text.start")]]
  UInt32 Bootstrap::GetBootstrapPageDirectoryPhysical() {
    return reinterpret_cast<UInt32>(_bootstrapPageDirectory);
  }

  void Bootstrap::TraceBootInfo() {
    Logger::WriteFormatted(
      Logger::Level::Debug,
      "BootInfo pre-paging: addr=%p entries=%u reserved=%p",
      _bootInfoPhysical,
      _bootInfoEntryCount,
      _bootInfoReserved
    );
    Logger::WriteFormatted(
      Logger::Level::Debug,
      "INIT.BND pre-paging: phys=%p size=0x%x magic0=0x%x magic1=0x%x",
      _bootInitBundlePhys,
      _bootInitBundleSize,
      _bootInitBundleMagic0,
      _bootInitBundleMagic1
    );
    Logger::WriteFormatted(
      Logger::Level::Debug,
      "INIT.BND pre-payload: off=0x2000 head0=0x%x head1=0x%x",
      _bootInitBundlePayload0,
      _bootInitBundlePayload1
    );

    if (_bootInitBundlePhys != 0 && _bootInitBundleSize >= 8) {
      const UInt8* magicBase
        = reinterpret_cast<const UInt8*>(_bootInitBundlePhys);

      UInt32 liveMagic0 = *reinterpret_cast<const UInt32*>(magicBase);
      UInt32 liveMagic1 = *reinterpret_cast<const UInt32*>(magicBase + 4);

      Logger::WriteFormatted(
        Logger::Level::Debug,
        "INIT.BND live pre-mm: phys=%p magic0=0x%x magic1=0x%x",
        _bootInitBundlePhys,
        liveMagic0,
        liveMagic1
      );
    }
  }
}
