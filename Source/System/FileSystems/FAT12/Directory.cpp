/**
 * @file System/FileSystems/FAT12/Directory.cpp
 * @brief FAT12 file system directory helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/Devices/BlockDevice.hpp>

#include "Directory.hpp"
#include "FAT.hpp"
#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12 {
  using BlockDevice = ABI::Devices::BlockDevice;
  using FileSystem = ABI::FileSystem;

  void Directory::Initialize(Volume& volume) {
    _volume = &volume;
  }

  bool Directory::ReadRootRecord(
    UInt32 index,
    Record& record,
    bool& end
  ) {
    end = false;

    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt8 sector[512] = {};
    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 rawCount = _volume->_rootEntryCount;
    UInt32 currentSector = 0xFFFFFFFF;
    UInt32 logicalIndex = 0;
    LFNState lfn {};

    ClearLFN(lfn);

    for (UInt32 rawIndex = 0; rawIndex < rawCount; ++rawIndex) {
      UInt32 sectorIndex = rawIndex / entriesPerSector;
      UInt32 entryIndex = rawIndex % entriesPerSector;

      if (sectorIndex >= _volume->_rootDirectorySectors) {
        end = true;

        return false;
      }

      if (sectorIndex != currentSector) {
        BlockDevice::Request request {};

        request.deviceId = _volume->_device.id;
        request.lba = _volume->_rootDirectoryStartLBA + sectorIndex;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        currentSector = sectorIndex;
      }

      const UInt8* base = sector + (entryIndex * 32);
      UInt8 first = base[0];

      if (first == 0x00) {
        end = true;

        return false;
      }

      if (first == 0xE5) {
        ClearLFN(lfn);

        continue;
      }

      UInt8 attributes = base[11];

      if (attributes == 0x0F) {
        ParseLFNEntry(base, lfn);

        continue;
      }

      if ((attributes & 0x08) != 0) {
        ClearLFN(lfn);

        continue;
      }

      if (logicalIndex == index) {
        PopulateRecord(_volume, base, lfn, record);
        ClearLFN(lfn);

        return true;
      }

      ++logicalIndex;

      ClearLFN(lfn);
    }

    end = true;

    return false;
  }

  bool Directory::BuildShortName(CString name, UInt8* outName) {
    if (!name || !outName) {
      return false;
    }

    for (UInt32 i = 0; i < 11; ++i) {
      outName[i] = ' ';
    }

    bool inExtension = false;
    UInt32 index = 0;
    UInt32 outIndex = 0;

    while (name[index] != '\0') {
      char ch = name[index];

      if (ch == '.') {
        inExtension = true;
        outIndex = 8;
        ++index;

        continue;
      }

      if (ch >= 'a' && ch <= 'z') {
        ch = static_cast<char>(ch - 'a' + 'A');
      }

      if (!inExtension) {
        if (outIndex >= 8) {
          return false;
        }
      } else {
        if (outIndex >= 11) {
          return false;
        }
      }

      outName[outIndex++] = static_cast<UInt8>(ch);
      ++index;
    }

    return true;
  }

  bool Directory::ReadRecord(
    UInt32 startCluster,
    UInt32 index,
    Record& record,
    bool& end
  ) {
    end = false;

    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    if (startCluster < 2) {
      end = true;

      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 cluster = startCluster;
    UInt32 logicalIndex = 0;
    LFNState lfn {};

    ClearLFN(lfn);

    for (;;) {
      UInt32 baseLBA
        = _volume->_dataStartLBA
        + (cluster - 2) * _volume->_sectorsPerCluster;

      for (
        UInt32 sectorIndex = 0;
        sectorIndex < _volume->_sectorsPerCluster;
        ++sectorIndex
      ) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _volume->_device.id;
        request.lba = baseLBA + sectorIndex;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        for (
          UInt32 entryIndex = 0;
          entryIndex < entriesPerSector;
          ++entryIndex
        ) {
          const UInt8* base = sector + (entryIndex * 32);
          UInt8 first = base[0];

          if (first == 0x00) {
            end = true;

            return false;
          }

          if (first == 0xE5) {
            ClearLFN(lfn);

            continue;
          }

          UInt8 attributes = base[11];

          if (attributes == 0x0F) {
            ParseLFNEntry(base, lfn);

            continue;
          }

          if ((attributes & 0x08) != 0) {
            ClearLFN(lfn);

            continue;
          }

          if (logicalIndex == index) {
            PopulateRecord(_volume, base, lfn, record);
            ClearLFN(lfn);

            return true;
          }

          ++logicalIndex;

          ClearLFN(lfn);
        }
      }

      UInt32 nextCluster = 0;

      if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        end = true;

        return false;
      }

      cluster = nextCluster;
    }

    end = true;

    return false;
  }

  bool Directory::ReadRecordAt(UInt32 lba, UInt32 offset, Record& record) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    if (offset > bytesPerSector - 32) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    const UInt8* base = sector + offset;
    UInt8 first = base[0];

    if (first == 0x00 || first == 0xE5) {
      return false;
    }

    UInt8 attributes = base[11];

    if (attributes == 0x0F || (attributes & 0x08) != 0) {
      return false;
    }

    LFNState lfn {};

    ClearLFN(lfn);
    PopulateRecord(_volume, base, lfn, record);

    return true;
  }

  bool Directory::RecordToEntry(
    const Record& record,
    FileSystem::DirectoryEntry& entry
  ) {
    if (record.longName[0] != '\0') {
      UInt32 limit = sizeof(entry.name) - 1;

      for (UInt32 i = 0; i < limit; ++i) {
        entry.name[i] = record.longName[i];

        if (record.longName[i] == '\0') {
          break;
        }
      }

      entry.name[limit] = '\0';
    } else {
      bool hasExtension = false;
      bool inExtension = false;
      UInt32 outIndex = 0;

      for (UInt32 i = 8; i < 11; ++i) {
        if (record.name[i] != ' ') {
          hasExtension = true;

          break;
        }
      }

      for (UInt32 i = 0; i < 11; ++i) {
        UInt8 ch = record.name[i];

        if (i == 8) {
          if (hasExtension) {
            entry.name[outIndex++] = '.';
          }

          inExtension = true;
        }

        if (ch == ' ') {
          if (!inExtension) {
            continue;
          }

          break;
        }

        if (outIndex >= sizeof(entry.name) - 1) {
          break;
        }

        entry.name[outIndex++] = static_cast<char>(ch);
      }

      entry.name[outIndex] = '\0';
    }

    entry.attributes = record.attributes;
    entry.sizeBytes = record.sizeBytes;
    entry.createTime = record.createTime;
    entry.createDate = record.createDate;
    entry.accessDate = record.accessDate;
    entry.writeTime = record.writeTime;
    entry.writeDate = record.writeDate;

    return true;
  }

  bool Directory::FindEntry(
    UInt32 startCluster,
    bool isRoot,
    CString name,
    UInt32& outCluster,
    UInt8& outAttributes,
    UInt32& outSize
  ) {
    if (!_volume) {
      return false;
    }

    if (!name || name[0] == '\0') {
      return false;
    }

    bool end = false;
    Record record {};
    UInt32 index = 0;

    auto matchesName = [&](const Record& candidate) -> bool {
      FileSystem::DirectoryEntry entry {};

      if (RecordToEntry(candidate, entry)) {
        if (_volume->MatchName(entry.name, name)) {
          return true;
        }
      }

      if (candidate.longName[0] != '\0') {
        Record shortRecord = candidate;

        shortRecord.longName[0] = '\0';

        FileSystem::DirectoryEntry shortEntry {};

        if (RecordToEntry(shortRecord, shortEntry)) {
          return _volume->MatchName(shortEntry.name, name);
        }
      }

      return false;
    };

    for (;;) {
      bool ok = false;

      if (isRoot) {
        ok = ReadRootRecord(index, record, end);
      } else {
        ok = ReadRecord(startCluster, index, record, end);
      }

      if (ok && matchesName(record)) {
        outCluster = record.startCluster;
        outAttributes = record.attributes;
        outSize = record.sizeBytes;

        return true;
      }

      if (end) {
        break;
      }

      ++index;
    }

    return false;
  }

  bool Directory::FindEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    Record& record,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    if (!_volume) {
      return false;
    }

    if (!name || name[0] == '\0') {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;
    LFNState lfn {};

    ClearLFN(lfn);

    auto matchesName = [&](const Record& candidate) -> bool {
      FileSystem::DirectoryEntry entry {};

      if (RecordToEntry(candidate, entry)) {
        if (_volume->MatchName(entry.name, name)) {
          return true;
        }
      }

      if (candidate.longName[0] != '\0') {
        Record shortRecord = candidate;

        shortRecord.longName[0] = '\0';

        FileSystem::DirectoryEntry shortEntry {};

        if (RecordToEntry(shortRecord, shortEntry)) {
          return _volume->MatchName(shortEntry.name, name);
        }
      }

      return false;
    };

    if (parentIsRoot) {
      UInt8 sector[512] = {};
      UInt32 rawCount = _volume->_rootEntryCount;
      UInt32 currentSector = 0xFFFFFFFF;

      for (UInt32 rawIndex = 0; rawIndex < rawCount; ++rawIndex) {
        UInt32 sectorIndex = rawIndex / entriesPerSector;
        UInt32 entryIndex = rawIndex % entriesPerSector;

        if (sectorIndex >= _volume->_rootDirectorySectors) {
          return false;
        }

        if (sectorIndex != currentSector) {
          BlockDevice::Request request {};

          request.deviceId = _volume->_device.id;
          request.lba = _volume->_rootDirectoryStartLBA + sectorIndex;
          request.count = 1;
          request.buffer = sector;

          if (BlockDevice::Read(request) != 0) {
            return false;
          }

          currentSector = sectorIndex;
        }

        const UInt8* base = sector + (entryIndex * 32);
        UInt8 first = base[0];

        if (first == 0x00) {
          return false;
        }

        if (first == 0xE5) {
          ClearLFN(lfn);

          continue;
        }

        UInt8 attributes = base[11];

        if (attributes == 0x0F) {
          ParseLFNEntry(base, lfn);

          continue;
        }

        if ((attributes & 0x08) != 0) {
          ClearLFN(lfn);

          continue;
        }

        PopulateRecord(_volume, base, lfn, record);

        if (matchesName(record)) {
          outLBA = _volume->_rootDirectoryStartLBA + sectorIndex;
          outOffset = entryIndex * 32;

          ClearLFN(lfn);

          return true;
        }

        ClearLFN(lfn);
      }

      return false;
    }

    if (parentCluster < 2) {
      return false;
    }

    UInt32 cluster = parentCluster;

    for (;;) {
      UInt32 baseLBA
        = _volume->_dataStartLBA
        + (cluster - 2) * _volume->_sectorsPerCluster;

      for (
        UInt32 sectorIndex = 0;
        sectorIndex < _volume->_sectorsPerCluster;
        ++sectorIndex
      ) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _volume->_device.id;
        request.lba = baseLBA + sectorIndex;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        for (
          UInt32 entryIndex = 0;
          entryIndex < entriesPerSector;
          ++entryIndex
        ) {
          const UInt8* base = sector + (entryIndex * 32);
          UInt8 first = base[0];

          if (first == 0x00) {
            return false;
          }

          if (first == 0xE5) {
            ClearLFN(lfn);

            continue;
          }

          UInt8 attributes = base[11];

          if (attributes == 0x0F) {
            ParseLFNEntry(base, lfn);

            continue;
          }

          if ((attributes & 0x08) != 0) {
            ClearLFN(lfn);

            continue;
          }

          PopulateRecord(_volume, base, lfn, record);

          if (matchesName(record)) {
            outLBA = baseLBA + sectorIndex;
            outOffset = entryIndex * 32;

            ClearLFN(lfn);

            return true;
          }

          ClearLFN(lfn);
        }
      }

      UInt32 nextCluster = 0;

      if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        break;
      }

      cluster = nextCluster;
    }

    return false;
  }

  bool Directory::UpdateEntry(
    UInt32 lba,
    UInt32 offset,
    UInt16 startCluster,
    UInt32 sizeBytes
  ) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    _volume->WriteUInt16(sector + offset, 26, startCluster);
    _volume->WriteUInt32(sector + offset, 28, sizeBytes);

    WriteTimestamps(_volume, sector + offset, false, true, true);

    request.lba = lba;
    request.buffer = sector;

    return BlockDevice::Write(request) == 0;
  }

  bool Directory::WriteEntry(
    UInt32 lba,
    UInt32 offset,
    const UInt8* entryBytes
  ) {
    if (!entryBytes) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    for (UInt32 i = 0; i < 32; ++i) {
      sector[offset + i] = entryBytes[i];
    }

    request.lba = lba;
    request.buffer = sector;

    return BlockDevice::Write(request) == 0;
  }

  bool Directory::FindFreeSlot(
    UInt32 startCluster,
    bool isRoot,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    if (isRoot) {
      UInt32 entryCount = _volume->_rootEntryCount;
      UInt32 entriesPerSector = bytesPerSector / 32;
      UInt32 sectorCount = _volume->_rootDirectorySectors;

      for (UInt32 sectorIndex = 0; sectorIndex < sectorCount; ++sectorIndex) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _volume->_device.id;
        request.lba = _volume->_rootDirectoryStartLBA + sectorIndex;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        for (
          UInt32 entryIndex = 0;
          entryIndex < entriesPerSector;
          ++entryIndex
        ) {
          UInt32 absoluteIndex = sectorIndex * entriesPerSector + entryIndex;

          if (absoluteIndex >= entryCount) {
            return false;
          }

          UInt8 first = sector[entryIndex * 32];

          if (first == 0x00 || first == 0xE5) {
            outLBA = _volume->_rootDirectoryStartLBA + sectorIndex;
            outOffset = entryIndex * 32;

            return true;
          }
        }
      }

      return false;
    }

    UInt32 cluster = startCluster;

    for (;;) {
      UInt32 baseLBA
        = _volume->_dataStartLBA
        + (cluster - 2) * _volume->_sectorsPerCluster;

      for (
        UInt32 sectorIndex = 0;
        sectorIndex < _volume->_sectorsPerCluster;
        ++sectorIndex
      ) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _volume->_device.id;
        request.lba = baseLBA + sectorIndex;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        UInt32 entriesPerSector = bytesPerSector / 32;

        for (
          UInt32 entryIndex = 0;
          entryIndex < entriesPerSector;
          ++entryIndex
        ) {
          UInt8 first = sector[entryIndex * 32];

          if (first == 0x00 || first == 0xE5) {
            outLBA = baseLBA + sectorIndex;
            outOffset = entryIndex * 32;

            return true;
          }
        }
      }

      UInt32 nextCluster = 0;

      if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        break;
      }

      cluster = nextCluster;
    }

    return false;
  }

  bool Directory::IsEmpty(UInt32 startCluster) {
    bool end = false;
    Record record {};
    UInt32 index = 0;

    for (;;) {
      if (ReadRecord(startCluster, index, record, end)) {
        if (!IsDotRecord(record)) {
          return false;
        }
      }

      if (end) {
        break;
      }

      ++index;
    }

    return true;
  }

  bool Directory::IsDotRecord(const Record& record) {
    if (record.name[0] != '.') {
      return false;
    }

    if (record.name[1] == ' ' || record.name[1] == '\0') {
      return true;
    }

    return record.name[1] == '.';
  }

  bool Directory::CreateDirectory(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt8 existingAttributes = 0;
    UInt32 existingCluster = 0;
    UInt32 existingSize = 0;

    if (
      FindEntry(
        parentCluster,
        parentIsRoot,
        name,
        existingCluster,
        existingAttributes,
        existingSize
      )
    ) {
      return false;
    }

    UInt8 shortName[11] = {};
    bool hasShortName = BuildShortName(name, shortName);
    bool needsLFN = !hasShortName;

    if (!hasShortName) {
      if (!BuildShortAlias(name, shortName)) {
        return false;
      }
    }

    UInt32 newCluster = 0;

    if (!_volume || !_volume->FindFreeCluster(newCluster)) {
      return false;
    }

    if (!_volume || !_volume->WriteFATEntry(newCluster, 0xFFF)) {
      return false;
    }
    if (_volume->_freeClusters > 0) {
      --_volume->_freeClusters;
      _volume->_info.freeSectors
        = _volume->_freeClusters * _volume->_sectorsPerCluster;
    }

    UInt8 zeroSector[512] = {};
    UInt32 clusterLBA
      = _volume->_dataStartLBA + (newCluster - 2) * _volume->_sectorsPerCluster;

    for (UInt32 s = 0; s < _volume->_sectorsPerCluster; ++s) {
      BlockDevice::Request request {};

      request.deviceId = _volume->_device.id;
      request.lba = clusterLBA + s;
      request.count = 1;
      request.buffer = zeroSector;

      if (BlockDevice::Write(request) != 0) {
        return false;
      }
    }

    UInt8 dirSector[512] = {};
    UInt8 dotName[11]
      = { '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
    UInt8 dotDotName[11]
      = { '.', '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };

    for (UInt32 i = 0; i < 11; ++i) {
      dirSector[i] = dotName[i];
    }

    dirSector[11] = 0x10;

    WriteTimestamps(_volume, dirSector, true, true, true);

    _volume->WriteUInt16(dirSector, 26, static_cast<UInt16>(newCluster));
    _volume->WriteUInt32(dirSector, 28, 0);

    for (UInt32 i = 0; i < 11; ++i) {
      dirSector[32 + i] = dotDotName[i];
    }

    dirSector[32 + 11] = 0x10;

    WriteTimestamps(_volume, dirSector + 32, true, true, true);

    _volume->WriteUInt16(
      dirSector,
      32 + 26,
      static_cast<UInt16>(parentIsRoot ? 0 : parentCluster)
    );
    _volume->WriteUInt32(dirSector, 32 + 28, 0);

    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = clusterLBA;
    request.count = 1;
    request.buffer = dirSector;

    if (BlockDevice::Write(request) != 0) {
      return false;
    }

    UInt32 entryLBA = 0;
    UInt32 entryOffset = 0;
    UInt32 entryIndex = 0;
    UInt32 lfnCount = 0;

    if (needsLFN) {
      UInt32 nameLen = 0;

      while (
        name[nameLen] != '\0'
        && nameLen < FileSystem::maxDirectoryLength - 1
      ) {
        ++nameLen;
      }

      lfnCount = (nameLen + 12) / 13;
    }

    UInt32 slotCount = needsLFN ? (lfnCount + 1) : 1;

    if (
      !FindFreeSlotRun(
        parentCluster,
        parentIsRoot,
        slotCount,
        entryIndex
      )
    ) {
      return false;
    }

    UInt32 shortEntryIndex = needsLFN ? (entryIndex + lfnCount) : entryIndex;

    if (
      !ComputeEntryLocation(
        parentCluster,
        parentIsRoot,
        shortEntryIndex,
        entryLBA,
        entryOffset
      )
    ) {
      return false;
    }

    if (needsLFN) {
      if (
        !WriteLFNEntries(
          parentCluster,
          parentIsRoot,
          entryIndex,
          name,
          shortName
        )
      ) {
        return false;
      }
    }

    UInt8 entryBytes[32] = {};

    for (UInt32 i = 0; i < 11; ++i) {
      entryBytes[i] = shortName[i];
    }

    entryBytes[11] = 0x10;

    WriteTimestamps(_volume, entryBytes, true, true, true);

    _volume->WriteUInt16(entryBytes, 26, static_cast<UInt16>(newCluster));
    _volume->WriteUInt32(entryBytes, 28, 0);

    return WriteEntry(entryLBA, entryOffset, entryBytes);
  }

  bool Directory::CreateFile(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt8 existingAttributes = 0;
    UInt32 existingCluster = 0;
    UInt32 existingSize = 0;

    if (
      FindEntry(
        parentCluster,
        parentIsRoot,
        name,
        existingCluster,
        existingAttributes,
        existingSize
      )
    ) {
      return false;
    }

    UInt8 shortName[11] = {};
    bool hasShortName = BuildShortName(name, shortName);
    bool needsLFN = !hasShortName;

    if (!hasShortName) {
      if (!BuildShortAlias(name, shortName)) {
        return false;
      }
    }

    UInt32 entryLBA = 0;
    UInt32 entryOffset = 0;
    UInt32 entryIndex = 0;
    UInt32 lfnCount = 0;

    if (needsLFN) {
      UInt32 nameLen = 0;

      while (
        name[nameLen] != '\0'
        && nameLen < FileSystem::maxDirectoryLength - 1
      ) {
        ++nameLen;
      }

      lfnCount = (nameLen + 12) / 13;
    }

    UInt32 slotCount = needsLFN ? (lfnCount + 1) : 1;

    if (
      !FindFreeSlotRun(
        parentCluster,
        parentIsRoot,
        slotCount,
        entryIndex
      )
    ) {
      return false;
    }

    UInt32 shortEntryIndex = needsLFN ? (entryIndex + lfnCount) : entryIndex;

    if (
      !ComputeEntryLocation(
        parentCluster,
        parentIsRoot,
        shortEntryIndex,
        entryLBA,
        entryOffset
      )
    ) {
      return false;
    }

    if (needsLFN) {
      if (
        !WriteLFNEntries(
          parentCluster,
          parentIsRoot,
          entryIndex,
          name,
          shortName
        )
      ) {
        return false;
      }
    }

    UInt8 entryBytes[32] = {};

    for (UInt32 i = 0; i < 11; ++i) {
      entryBytes[i] = shortName[i];
    }

    entryBytes[11] = 0x20;

    WriteTimestamps(_volume, entryBytes, true, true, true);

    _volume->WriteUInt16(entryBytes, 26, 0);
    _volume->WriteUInt32(entryBytes, 28, 0);

    return WriteEntry(entryLBA, entryOffset, entryBytes);
  }

  bool Directory::RemoveEntry(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    Record record {};
    UInt32 lba = 0;
    UInt32 offset = 0;

    if (
      !FindEntryLocation(
        parentCluster,
        parentIsRoot,
        name,
        record,
        lba,
        offset
      )
    ) {
      return false;
    }

    if ((record.attributes & 0x10) != 0) {
      if (!IsEmpty(record.startCluster)) {
        return false;
      }
    }

    if (!_volume || !_volume->FreeClusterChain(record.startCluster)) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    WriteTimestamps(_volume, sector + offset, false, true, true);

    sector[offset] = 0xE5;

    request.lba = lba;
    request.buffer = sector;

    return BlockDevice::Write(request) == 0;
  }

  bool Directory::RenameEntry(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    CString newName
  ) {
    Record record {};
    UInt32 lba = 0;
    UInt32 offset = 0;

    if (
      !FindEntryLocation(
        parentCluster,
        parentIsRoot,
        name,
        record,
        lba,
        offset
      )
    ) {
      return false;
    }

    UInt8 shortName[11] = {};

    if (!BuildShortName(newName, shortName)) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    for (UInt32 i = 0; i < 11; ++i) {
      sector[offset + i] = shortName[i];
    }

    WriteTimestamps(_volume, sector + offset, false, true, true);

    request.lba = lba;
    request.buffer = sector;

    return BlockDevice::Write(request) == 0;
  }

  bool Directory::GetEntryInfo(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    FileSystem::FileInfo& outInfo,
    UInt8& outAttributes
  ) {
    Record record {};
    UInt32 lba = 0;
    UInt32 offset = 0;

    if (
      !FindEntryLocation(
        parentCluster,
        parentIsRoot,
        name,
        record,
        lba,
        offset
      )
    ) {
      return false;
    }

    outInfo.sizeBytes = record.sizeBytes;
    outInfo.attributes = record.attributes;
    outInfo.createTime = record.createTime;
    outInfo.createDate = record.createDate;
    outInfo.accessDate = record.accessDate;
    outInfo.writeTime = record.writeTime;
    outInfo.writeDate = record.writeDate;
    outAttributes = record.attributes;

    return true;
  }

  void Directory::ClearLFN(LFNState& state) {
    state.checksum = 0;
    state.expected = 0;
    state.seenMask = 0;
    state.active = false;
    state.name[0] = '\0';
  }

  UInt8 Directory::LFNChecksum(const UInt8* shortName) {
    UInt8 sum = 0;

    for (UInt32 i = 0; i < 11; ++i) {
      sum = static_cast<UInt8>(((sum & 1) ? 0x80 : 0) + (sum >> 1));
      sum = static_cast<UInt8>(sum + shortName[i]);
    }

    return sum;
  }

  void Directory::CopyLFNChars(
    LFNState& state,
    UInt32 offset,
    const UInt8* base,
    UInt32 count
  ) {
    UInt32 maxChars = FileSystem::maxDirectoryLength - 1;

    for (UInt32 i = 0; i < count; ++i) {
      UInt16 value = static_cast<UInt16>(
        base[i * 2] | (static_cast<UInt16>(base[i * 2 + 1]) << 8)
      );

      if (value == 0x0000 || value == 0xFFFF) {
        if (offset + i < FileSystem::maxDirectoryLength) {
          state.name[offset + i] = '\0';
        }

        return;
      }

      if (offset + i >= maxChars) {
        return;
      }

      state.name[offset + i]
        = value <= 0x7F ? static_cast<char>(value) : '?';
    }
  }

  void Directory::ParseLFNEntry(const UInt8* base, LFNState& state) {
    UInt8 order = static_cast<UInt8>(base[0] & 0x1F);
    UInt8 checksum = base[13];

    if (order == 0) {
      ClearLFN(state);

      return;
    }

    if ((base[0] & 0x40) != 0) {
      ClearLFN(state);

      state.active = true;
      state.checksum = checksum;
      state.expected = order;

      for (UInt32 i = 0; i < FileSystem::maxDirectoryLength; ++i) {
        state.name[i] = '\0';
      }
    }

    if (!state.active || checksum != state.checksum) {
      ClearLFN(state);

      return;
    }

    UInt32 maxSegments
      = (FileSystem::maxDirectoryLength + 12) / 13;

    if (order > maxSegments) {
      return;
    }

    UInt32 offset = (order - 1) * 13;

    CopyLFNChars(state, offset, base + 1, 5);
    CopyLFNChars(state, offset + 5, base + 14, 6);
    CopyLFNChars(state, offset + 11, base + 28, 2);

    state.seenMask = static_cast<UInt8>(state.seenMask | (1u << (order - 1)));
  }

  bool Directory::UseLFN(const LFNState& state, const UInt8* shortName) {
    if (!state.active || state.name[0] == '\0') {
      return false;
    }

    return LFNChecksum(shortName) == state.checksum;
  }

  void Directory::PopulateRecord(
    Volume* volume,
    const UInt8* base,
    const LFNState& lfn,
    Record& record
  ) {
    if (!volume || !base) {
      return;
    }

    for (UInt32 i = 0; i < sizeof(record.name); ++i) {
      record.name[i] = base[i];
    }

    record.longName[0] = '\0';

    if (UseLFN(lfn, record.name)) {
      UInt32 limit = sizeof(record.longName) - 1;

      for (UInt32 i = 0; i < limit; ++i) {
        record.longName[i] = lfn.name[i];

        if (lfn.name[i] == '\0') {
          break;
        }
      }

      record.longName[limit] = '\0';
    }

    record.attributes = base[11];
    record.createTime = volume->ReadUInt16(base, 14);
    record.createDate = volume->ReadUInt16(base, 16);
    record.accessDate = volume->ReadUInt16(base, 18);
    record.writeTime = volume->ReadUInt16(base, 22);
    record.writeDate = volume->ReadUInt16(base, 24);
    record.startCluster = volume->ReadUInt16(base, 26);
    record.sizeBytes = volume->ReadUInt32(base, 28);
  }

  void Directory::WriteTimestamps(
    Volume* volume,
    UInt8* entryBytes,
    bool setCreate,
    bool setAccess,
    bool setWrite
  ) {
    if (!volume || !entryBytes) {
      return;
    }

    // fixed timestamp until we expose a real clock
    UInt16 date = static_cast<UInt16>((45u << 9) | (1u << 5) | 1u);
    UInt16 time = 0;

    if (setCreate) {
      volume->WriteUInt16(entryBytes, 14, time);
      volume->WriteUInt16(entryBytes, 16, date);
    }

    if (setAccess) {
      volume->WriteUInt16(entryBytes, 18, date);
    }

    if (setWrite) {
      volume->WriteUInt16(entryBytes, 22, time);
      volume->WriteUInt16(entryBytes, 24, date);
    }
  }

  bool Directory::BuildShortAlias(CString name, UInt8* outName) {
    if (!name || !outName) {
      return false;
    }

    for (UInt32 i = 0; i < 11; ++i) {
      outName[i] = ' ';
    }

    bool inExtension = false;
    char base[16] = {};
    char ext[8] = {};
    UInt32 baseLen = 0;
    UInt32 extLen = 0;

    for (UInt32 i = 0; name[i] != '\0'; ++i) {
      char ch = name[i];

      if (ch == '.') {
        if (!inExtension) {
          inExtension = true;
        }

        continue;
      }

      if (ch == ' ') {
        continue;
      }

      if (ch >= 'a' && ch <= 'z') {
        ch = static_cast<char>(ch - 'a' + 'A');
      }

      if (
        (ch < 'A' || ch > 'Z')
        && (ch < '0' || ch > '9')
      ) {
        ch = '_';
      }

      if (!inExtension) {
        if (baseLen < sizeof(base) - 1) {
          base[baseLen++] = ch;
        }
      } else {
        if (extLen < sizeof(ext) - 1) {
          ext[extLen++] = ch;
        }
      }
    }

    if (baseLen == 0) {
      return false;
    }

    UInt32 outIndex = 0;
    UInt32 copyLen = baseLen > 6 ? 6 : baseLen;

    for (UInt32 i = 0; i < copyLen; ++i) {
      outName[outIndex++] = static_cast<UInt8>(base[i]);
    }

    if (outIndex + 1 < 8) {
      outName[outIndex++] = '~';
      outName[outIndex++] = '1';
    }

    for (UInt32 i = outIndex; i < 8; ++i) {
      outName[i] = ' ';
    }

    for (UInt32 i = 0; i < 3 && i < extLen; ++i) {
      outName[8 + i] = static_cast<UInt8>(ext[i]);
    }

    return true;
  }

  bool Directory::ComputeEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    UInt32 entryIndex,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;

    if (parentIsRoot) {
      UInt32 sectorIndex = entryIndex / entriesPerSector;
      UInt32 entryOffset = entryIndex % entriesPerSector;

      if (sectorIndex >= _volume->_rootDirectorySectors) {
        return false;
      }

      outLBA = _volume->_rootDirectoryStartLBA + sectorIndex;
      outOffset = entryOffset * 32;

      return true;
    }

    if (parentCluster < 2) {
      return false;
    }

    UInt32 entriesPerCluster
      = entriesPerSector * _volume->_sectorsPerCluster;
    UInt32 clusterIndex = entryIndex / entriesPerCluster;
    UInt32 clusterEntryIndex = entryIndex % entriesPerCluster;
    UInt32 cluster = parentCluster;

    for (UInt32 i = 0; i < clusterIndex; ++i) {
      UInt32 nextCluster = 0;

      if (!_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        return false;
      }

      cluster = nextCluster;
    }

    UInt32 sectorIndex = clusterEntryIndex / entriesPerSector;
    UInt32 entryOffset = clusterEntryIndex % entriesPerSector;

    outLBA
      = _volume->_dataStartLBA
      + (cluster - 2) * _volume->_sectorsPerCluster
      + sectorIndex;
    outOffset = entryOffset * 32;

    return true;
  }

  bool Directory::FindFreeSlotRun(
    UInt32 startCluster,
    bool isRoot,
    UInt32 count,
    UInt32& outIndex
  ) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    if (count == 0) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 runCount = 0;
    UInt32 runStart = 0;

    if (isRoot) {
      UInt32 entryCount = _volume->_rootEntryCount;
      UInt8 sector[512] = {};
      UInt32 currentSector = 0xFFFFFFFF;

      for (UInt32 index = 0; index < entryCount; ++index) {
        UInt32 sectorIndex = index / entriesPerSector;
        UInt32 entryIndex = index % entriesPerSector;

        if (sectorIndex >= _volume->_rootDirectorySectors) {
          break;
        }

        if (sectorIndex != currentSector) {
          BlockDevice::Request request {};

          request.deviceId = _volume->_device.id;
          request.lba = _volume->_rootDirectoryStartLBA + sectorIndex;
          request.count = 1;
          request.buffer = sector;

          if (BlockDevice::Read(request) != 0) {
            return false;
          }

          currentSector = sectorIndex;
        }

        UInt8 first = sector[entryIndex * 32];
        bool free = first == 0x00 || first == 0xE5;

        if (free) {
          if (runCount == 0) {
            runStart = index;
          }

          ++runCount;

          if (runCount >= count) {
            outIndex = runStart;

            return true;
          }
        } else {
          runCount = 0;
        }
      }

      return false;
    }

    if (startCluster < 2) {
      return false;
    }

    UInt32 cluster = startCluster;
    UInt32 index = 0;

    for (;;) {
      UInt32 baseLBA
        = _volume->_dataStartLBA
        + (cluster - 2) * _volume->_sectorsPerCluster;

      for (
        UInt32 sectorIndex = 0;
        sectorIndex < _volume->_sectorsPerCluster;
        ++sectorIndex
      ) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _volume->_device.id;
        request.lba = baseLBA + sectorIndex;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        for (UInt32 entryIndex = 0; entryIndex < entriesPerSector; ++entryIndex) {
          UInt8 first = sector[entryIndex * 32];
          bool free = first == 0x00 || first == 0xE5;

          if (free) {
            if (runCount == 0) {
              runStart = index;
            }

            ++runCount;

            if (runCount >= count) {
              outIndex = runStart;

              return true;
            }
          } else {
            runCount = 0;
          }

          ++index;
        }
      }

      UInt32 nextCluster = 0;

      if (!_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        break;
      }

      cluster = nextCluster;
    }

    return false;
  }

  bool Directory::WriteLFNEntries(
    UInt32 parentCluster,
    bool parentIsRoot,
    UInt32 entryIndex,
    CString name,
    const UInt8* shortName
  ) {
    if (!name || !shortName) {
      return false;
    }

    UInt32 nameLen = 0;

    while (name[nameLen] != '\0' && nameLen < FileSystem::maxDirectoryLength - 1) {
      ++nameLen;
    }

    if (nameLen == 0) {
      return false;
    }

    UInt32 segmentCount = (nameLen + 12) / 13;
    UInt8 checksum = LFNChecksum(shortName);
    static const UInt8 lfnOffsets[13]
      = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };

    for (UInt32 entry = 0; entry < segmentCount; ++entry) {
      UInt8 order = static_cast<UInt8>(segmentCount - entry);

      if (entry == 0) {
        order = static_cast<UInt8>(order | 0x40);
      }

      UInt8 entryBytes[32] = {};

      entryBytes[0] = order;
      entryBytes[11] = 0x0F;
      entryBytes[12] = 0x00;
      entryBytes[13] = checksum;
      entryBytes[26] = 0x00;
      entryBytes[27] = 0x00;

      UInt32 baseIndex = (static_cast<UInt32>(order & 0x1F) - 1) * 13;

      for (UInt32 i = 0; i < 13; ++i) {
        UInt32 nameIndex = baseIndex + i;
        UInt16 value = 0xFFFF;

        if (nameIndex < nameLen) {
          value = static_cast<UInt16>(name[nameIndex]);
        } else if (nameIndex == nameLen) {
          value = 0x0000;
        }

        UInt8 offset = lfnOffsets[i];

        entryBytes[offset] = static_cast<UInt8>(value & 0xFF);
        entryBytes[offset + 1] = static_cast<UInt8>(value >> 8);
      }

      UInt32 lba = 0;
      UInt32 offset = 0;

      if (
        !ComputeEntryLocation(
          parentCluster,
          parentIsRoot,
          entryIndex + entry,
          lba,
          offset
        )
      ) {
        return false;
      }

      if (!WriteEntry(lba, offset, entryBytes)) {
        return false;
      }
    }

    return true;
  }
}

