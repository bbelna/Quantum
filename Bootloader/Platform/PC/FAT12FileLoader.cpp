/**
 * @file Bootloader/Platform/PC/FAT12FileLoader.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::FAT12FileLoader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FAT12FileLoader.hpp"

namespace Quantum::Bootloader::Platform::PC {
  void FAT12FileLoader::Initialize(UInt8 bootDrive) {
    _bootDrive = bootDrive;

    // parse BPB fields from physical 0x7C00 (stage 1 still in memory)
    UInt8* bpb = reinterpret_cast<UInt8*>(_bpbBase);

    _bytesPerSector = *reinterpret_cast<UInt16*>(bpb + 11);
    _sectorsPerCluster = bpb[13];
    _reservedSectors = *reinterpret_cast<UInt16*>(bpb + 14);
    _numberOfFATs = bpb[16];
    _rootEntries = *reinterpret_cast<UInt16*>(bpb + 17);
    _sectorsPerFAT = *reinterpret_cast<UInt16*>(bpb + 22);
    _sectorsPerTrack = *reinterpret_cast<UInt16*>(bpb + 24);
    _numberOfHeads = *reinterpret_cast<UInt16*>(bpb + 26);

    // compute derived values
    _rootDirectorySectors = (_rootEntries * 32 + _bytesPerSector - 1)
                          / _bytesPerSector;
    _firstRootSector = _reservedSectors + _numberOfFATs * _sectorsPerFAT;
    _firstDataSector = _firstRootSector + _rootDirectorySectors;

    // reset disk controller
    BIOSRegisters regs = {};

    regs.EAX = 0;
    regs.EDX = _bootDrive;

    CallBIOS(0x13, &regs);

    _initialized = true;
  }

  void FAT12FileLoader::_toFAT12Name(
    const char* component,
    Size length,
    char* out
  ) {
    for (Size i = 0; i < 11; i++) out[i] = ' ';

    Size dotPos = length;

    for (Size i = 0; i < length; i++) {
      if (component[i] == '.') {
        dotPos = i;

        break;
      }
    }

    Size nameLen = dotPos < 8 ? dotPos : 8;

    for (Size i = 0; i < nameLen; i++) {
      char c = component[i];

      if (c >= 'a' && c <= 'z') c -= 32;

      out[i] = c;
    }

    if (dotPos < length) {
      Size extStart = dotPos + 1;
      Size extLen = length - extStart;

      if (extLen > 3) extLen = 3;

      for (Size i = 0; i < extLen; i++) {
        char c = component[extStart + i];

        if (c >= 'a' && c <= 'z') c -= 32;

        out[8 + i] = c;
      }
    }
  }

  Size FAT12FileLoader::_splitPath(
    const char* path,
    const char* components[MaxPathComponents],
    Size lengths[MaxPathComponents]
  ) {
    Size count = 0;

    if (*path == '/') path++;

    while (*path && count < MaxPathComponents) {
      components[count] = path;

      Size len = 0;

      while (path[len] && path[len] != '/') len++;

      lengths[count] = len;
      count++;
      path += len;

      if (*path == '/') path++;
    }

    return count;
  }

  bool FAT12FileLoader::_readSector(
    UInt16 lba,
    UInt32 destinationPhysicalAddress
  ) const {
    // BIOS INT 13h uses real-mode ES:BX addressing, which cannot reach
    // memory above the 1 MB mark.  For high-memory destinations, read
    // into a low-memory bounce buffer and copy up in protected mode.
    bool bounce = destinationPhysicalAddress >= _realModeAddressLimit;
    UInt32 biosTarget = bounce ? _bounceBuffer : destinationPhysicalAddress;

    BIOSRegisters regs = {};

    if (_bootDrive >= 0x80) {
      // HDD: use INT 13h extensions (AH=42h) with a DAP in low memory.
      // The DAP is placed at a fixed low-memory address that the BIOS
      // can access in real mode.
      struct DiskAddressPacket {
        UInt8  Size;
        UInt8  Reserved;
        UInt16 Count;
        UInt16 Offset;
        UInt16 Segment;
        UInt32 LBALow;
        UInt32 LBAHigh;
      } __attribute__((packed));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
      auto* dap = reinterpret_cast<DiskAddressPacket*>(_dapBuffer);

      dap->Size = 16;
      dap->Reserved = 0;
      dap->Count = 1;
      dap->Offset = static_cast<UInt16>(biosTarget & 0xF);
      dap->Segment = static_cast<UInt16>(biosTarget >> 4);
      dap->LBALow = static_cast<UInt32>(lba);
      dap->LBAHigh = 0;
#pragma GCC diagnostic pop

      regs.EAX = 0x4200;
      regs.EDX = _bootDrive;
      regs.DS  = 0;
      regs.ESI = _dapBuffer;
    } else {
      // Floppy: use CHS addressing via INT 13h AH=02h.
      UInt16 track = lba / _sectorsPerTrack;
      UInt16 sector = (lba % _sectorsPerTrack) + 1;
      UInt16 head = track % _numberOfHeads;
      UInt16 cylinder = track / _numberOfHeads;

      UInt8 ch = cylinder & 0xFF;
      UInt8 cl = (sector & 0x3F) | ((cylinder >> 2) & 0xC0);

      regs.EAX = 0x0201;
      regs.ECX = (static_cast<UInt32>(ch) << 8) | cl;
      regs.EDX = (static_cast<UInt32>(head) << 8) | _bootDrive;
      regs.ES  = static_cast<UInt16>(biosTarget >> 4);
      regs.EBX = biosTarget & 0xF;
    }

    CallBIOS(0x13, &regs);

    if (regs.EFLAGS & 1) return false;

    if (bounce) {
      const UInt8* src = reinterpret_cast<const UInt8*>(_bounceBuffer);
      UInt8* dst = reinterpret_cast<UInt8*>(destinationPhysicalAddress);

      for (UInt16 i = 0; i < _bytesPerSector; i++) dst[i] = src[i];
    }

    return true;
  }

  UInt16 FAT12FileLoader::_getNextCluster(UInt16 cluster) const {
    UInt8* fat = reinterpret_cast<UInt8*>(_fatBuffer);
    UInt32 offset = cluster + (cluster / 2);
    UInt16 value = *reinterpret_cast<UInt16*>(fat + offset);

    if (cluster & 1) {
      return (value >> 4) & 0x0FFF;
    } else {
      return value & 0x0FFF;
    }
  }

  UInt16 FAT12FileLoader::_clusterToLBA(UInt16 cluster) const {
    return (cluster - 2) * _sectorsPerCluster + _firstDataSector;
  }

  bool FAT12FileLoader::_memCompare(
    const void* a,
    const void* b,
    UInt32 n
  ) const {
    const UInt8* pa = reinterpret_cast<const UInt8*>(a);
    const UInt8* pb = reinterpret_cast<const UInt8*>(b);

    for (UInt32 i = 0; i < n; i++) {
      if (pa[i] != pb[i]) return false;
    }

    return true;
  }

  bool FAT12FileLoader::_findFile(const char* name, FileInfo* out) const {
    UInt16 lba = _firstRootSector;

    for (UInt16 sector = 0; sector < _rootDirectorySectors; sector++) {
      if (!_readSector(lba + sector, _rootDirectoryBuffer)) return false;

      UInt8* entries = reinterpret_cast<UInt8*>(_rootDirectoryBuffer);
      UInt16 maxEntries = 16;

      for (UInt16 entryIndex = 0; entryIndex < maxEntries; entryIndex++) {
        UInt8* entry = entries + entryIndex * 32;
        UInt8 firstByte = entry[0];

        if (firstByte == 0x00) return false;
        else if (firstByte == 0xE5) continue;

        UInt8 attribute = entry[11];

        if (attribute & 0x08) continue;
        else if (attribute & 0x10) continue;

        if (_memCompare(entry, name, 11)) {
          UInt16 firstCluster = *reinterpret_cast<UInt16*>(entry + 26);

          if (firstCluster < 2) return false;

          UInt32 fileSize =
            static_cast<UInt32>(*reinterpret_cast<UInt16*>(entry + 28))
            | (static_cast<UInt32>(*reinterpret_cast<UInt16*>(entry + 30)) << 16);

          out->FirstCluster = firstCluster;
          out->Size = fileSize;
          out->Sectors = (fileSize + _bytesPerSector - 1) / _bytesPerSector;

          return true;
        }
      }
    }

    return false;
  }

  bool FAT12FileLoader::_findDirectory(const char* name, UInt16* outFirstCluster) const {
    UInt16 lba = _firstRootSector;

    for (UInt16 sector = 0; sector < _rootDirectorySectors; sector++) {
      if (!_readSector(lba + sector, _rootDirectoryBuffer)) return false;

      UInt8* entries = reinterpret_cast<UInt8*>(_rootDirectoryBuffer);
      UInt16 maxEntries = 16;

      for (UInt16 entryIndex = 0; entryIndex < maxEntries; entryIndex++) {
        UInt8* entry = entries + entryIndex * 32;
        UInt8 firstByte = entry[0];

        if (firstByte == 0x00) return false;
        else if (firstByte == 0xE5) continue;

        UInt8 attribute = entry[11];

        if (!(attribute & 0x10)) continue;
        if (attribute & 0x08) continue;

        if (_memCompare(entry, name, 11)) {
          *outFirstCluster = *reinterpret_cast<UInt16*>(entry + 26);

          return *outFirstCluster >= 2;
        }
      }
    }

    return false;
  }

  bool FAT12FileLoader::_findFileInDirectory(
    const char* name,
    UInt16 dirFirstCluster,
    FileInfo* out
  ) const {
    UInt16 cluster = dirFirstCluster;

    while (cluster >= 2 && cluster < 0x0FF8) {
      UInt16 lba = _clusterToLBA(cluster);

      for (UInt16 s = 0; s < _sectorsPerCluster; s++) {
        if (!_readSector(lba + s, _rootDirectoryBuffer)) return false;

        UInt8* entries = reinterpret_cast<UInt8*>(_rootDirectoryBuffer);
        UInt16 maxEntries = 16;

        for (UInt16 entryIndex = 0; entryIndex < maxEntries; entryIndex++) {
          UInt8* entry = entries + entryIndex * 32;
          UInt8 firstByte = entry[0];

          if (firstByte == 0x00) {
            return false;
          } else if (firstByte == 0xE5) {
            continue;
          }

          UInt8 attribute = entry[11];

          if ((attribute & 0x08) || (attribute & 0x10)) continue;

          if (_memCompare(entry, name, 11)) {
            UInt16 firstCluster = *reinterpret_cast<UInt16*>(entry + 26);

            if (firstCluster < 2) return false;

            UInt32 fileSize =
              static_cast<UInt32>(*reinterpret_cast<UInt16*>(entry + 28))
              | (static_cast<UInt32>(*reinterpret_cast<UInt16*>(entry + 30)) << 16);

            out->FirstCluster = firstCluster;
            out->Size = fileSize;
            out->Sectors = (fileSize + _bytesPerSector - 1) / _bytesPerSector;

            return true;
          }
        }
      }

      cluster = _getNextCluster(cluster);
    }

    return false;
  }

  bool FAT12FileLoader::_loadFile(
    const FileInfo& file,
    UInt32 destinationPhysicalAddress
  ) const {
    UInt16 cluster = file.FirstCluster;
    UInt16 sectorsRemaining = file.Sectors;

    while (sectorsRemaining > 0) {
      if (cluster >= 0x0FF8) {
        break;
      } else if (cluster < 2) {
        return false;
      }

      UInt16 lba = _clusterToLBA(cluster);
      UInt16 clusterSectors = _sectorsPerCluster;

      if (clusterSectors > sectorsRemaining) {
        clusterSectors = sectorsRemaining;
      }

      for (UInt16 i = 0; i < clusterSectors; i++) {
        if (!_readSector(lba + i, destinationPhysicalAddress)) {
          return false;
        }

        if (_spinner) _spinner->Tick();

        destinationPhysicalAddress += _bytesPerSector;
        sectorsRemaining--;
      }

      cluster = _getNextCluster(cluster);
    }

    return true;
  }

  bool FAT12FileLoader::Load(
    const char* path,
    UInt32 address,
    UInt32* outLoadedSize
  ) {
    if (outLoadedSize) *outLoadedSize = 0;
    if (!_initialized) return false;

    const char* components[MaxPathComponents];
    Size lengths[MaxPathComponents];
    Size count = _splitPath(path, components, lengths);

    if (count == 0) return false;

    if (count == 1) {
      char name[11];

      _toFAT12Name(components[0], lengths[0], name);

      FileInfo file;

      if (!_findFile(name, &file)) return false;
      if (outLoadedSize) *outLoadedSize = file.Size;
      if (_spinner) _spinner->Show();

      bool loaded = _loadFile(file, address);

      if (_spinner) _spinner->Hide();

      return loaded;
    }

    UInt16 dirCluster;
    char name[11];

    _toFAT12Name(components[0], lengths[0], name);

    if (!_findDirectory(name, &dirCluster)) return false;

    for (Size i = 1; i < count - 1; i++) {
      _toFAT12Name(components[i], lengths[i], name);

      return false;
    }

    _toFAT12Name(components[count - 1], lengths[count - 1], name);

    FileInfo file;

    if (!_findFileInDirectory(name, dirCluster, &file)) return false;
    if (outLoadedSize) *outLoadedSize = file.Size;
    if (_spinner) _spinner->Show();

    bool loaded = _loadFile(file, address);

    if (_spinner) _spinner->Hide();

    return loaded;
  }
}
