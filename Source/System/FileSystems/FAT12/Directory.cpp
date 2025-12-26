/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Directory.cpp
 * FAT12 directory helpers.
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

    if (index >= _volume->_rootEntryCount) {
      end = true;

      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 sectorIndex = index / entriesPerSector;
    UInt32 entryIndex = index % entriesPerSector;

    // check for end of root directory
    if (sectorIndex >= _volume->_rootDirectorySectors) {
      end = true;

      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = _volume->_rootDirectoryStartLBA + sectorIndex;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    const UInt8* base = sector + (entryIndex * 32);
    UInt8 first = base[0];

    // check for end-of-directory marker
    if (first == 0x00) {
      end = true;

      return false;
    }

    if (first == 0xE5) {
      return false;
    }

    UInt8 attributes = base[11];

    if (attributes == 0x0F) {
      return false;
    }

    if ((attributes & 0x08) != 0) {
      return false;
    }

    for (UInt32 i = 0; i < sizeof(record.name); ++i) {
      record.name[i] = base[i];
    }

    record.attributes = attributes;
    record.startCluster = _volume->ReadUInt16(base, 26);
    record.sizeBytes = _volume->ReadUInt32(base, 28);

    return true;
  }

  bool Directory::BuildShortName(CString name, UInt8* outName) {
    if (!name || !outName) {
      return false;
    }

    for (UInt32 i = 0; i < 11; ++i) {
      outName[i] = ' ';
    }

    UInt32 index = 0;
    UInt32 outIndex = 0;
    bool inExtension = false;

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

    UInt32 entriesPerCluster
      = (bytesPerSector / 32) * _volume->_sectorsPerCluster;
    UInt32 clusterIndex = index / entriesPerCluster;
    UInt32 entryIndex = index % entriesPerCluster;

    UInt32 cluster = startCluster;

    for (UInt32 i = 0; i < clusterIndex; ++i) {
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

    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 sectorIndex = entryIndex / entriesPerSector;
    UInt32 entryOffset = entryIndex % entriesPerSector;

    UInt32 lba
      = _volume->_dataStartLBA + (cluster - 2)
      * _volume->_sectorsPerCluster + sectorIndex;
    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    const UInt8* base = sector + (entryOffset * 32);
    UInt8 first = base[0];

    if (first == 0x00) {
      end = true;

      return false;
    }

    if (first == 0xE5) {
      return false;
    }

    UInt8 attributes = base[11];

    if (attributes == 0x0F) {
      return false;
    }

    if ((attributes & 0x08) != 0) {
      return false;
    }

    for (UInt32 i = 0; i < sizeof(record.name); ++i) {
      record.name[i] = base[i];
    }

    record.attributes = attributes;
    record.startCluster = _volume->ReadUInt16(base, 26);
    record.sizeBytes = _volume->ReadUInt32(base, 28);

    return true;
  }

  bool Directory::RecordToEntry(
    const Record& record,
    FileSystem::DirectoryEntry& entry
  ) {
    UInt32 outIndex = 0;
    bool hasExtension = false;
    bool inExtension = false;

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
    entry.attributes = record.attributes;
    entry.sizeBytes = record.sizeBytes;

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

    Record record {};
    bool end = false;
    UInt32 index = 0;

    for (;;) {
      bool ok = false;

      if (isRoot) {
        ok = ReadRootRecord(index, record, end);
      } else {
        ok = ReadRecord(startCluster, index, record, end);
      }

      if (ok) {
        FileSystem::DirectoryEntry entry {};

        if (RecordToEntry(record, entry)) {
          if (_volume->MatchName(entry.name, name)) {
            outCluster = record.startCluster;
            outAttributes = record.attributes;
            outSize = record.sizeBytes;

            return true;
          }
        }
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

    bool end = false;
    UInt32 index = 0;

    auto computeLocation = [&](
      UInt32 entryIndex,
      UInt32& lba,
      UInt32& offset
    ) -> bool {
      if (!_volume || !_volume->_valid) {
        return false;
      }

      UInt32 bytesPerSector = _volume->_info.sectorSize;

      if (bytesPerSector != 512) {
        // only support 512-byte sectors for now
        return false;
      }

      if (parentIsRoot) {
        UInt32 entriesPerSector = bytesPerSector / 32;
        UInt32 sectorIndex = entryIndex / entriesPerSector;
        UInt32 entryOffset = entryIndex % entriesPerSector;

        if (sectorIndex >= _volume->_rootDirectorySectors) {
          return false;
        }

        lba = _volume->_rootDirectoryStartLBA + sectorIndex;
        offset = entryOffset * 32;

        return true;
      }

      if (parentCluster < 2) {
        return false;
      }

      UInt32 entriesPerCluster
        = (bytesPerSector / 32) * _volume->_sectorsPerCluster;
      UInt32 clusterIndex = entryIndex / entriesPerCluster;
      UInt32 clusterEntryIndex = entryIndex % entriesPerCluster;
      UInt32 cluster = parentCluster;

      for (UInt32 i = 0; i < clusterIndex; ++i) {
        UInt32 nextCluster = 0;

        if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
          return false;
        }

        if (FAT::IsEndOfChain(nextCluster)) {
          return false;
        }

        cluster = nextCluster;
      }

      UInt32 entriesPerSector = bytesPerSector / 32;
      UInt32 sectorIndex = clusterEntryIndex / entriesPerSector;
      UInt32 entryOffset = clusterEntryIndex % entriesPerSector;

      lba
        = _volume->_dataStartLBA
        + (cluster - 2) * _volume->_sectorsPerCluster
        + sectorIndex;
      offset = entryOffset * 32;

      return true;
    };

    for (;;) {
      bool ok = false;

      if (parentIsRoot) {
        ok = ReadRootRecord(index, record, end);
      } else {
        ok = ReadRecord(parentCluster, index, record, end);
      }

      if (ok) {
        FileSystem::DirectoryEntry entry {};

        if (RecordToEntry(record, entry)) {
          if (_volume->MatchName(entry.name, name)) {
            return computeLocation(index, outLBA, outOffset);
          }
        }
      }

      if (end) {
        break;
      }

      ++index;
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

    UInt32 existingCluster = 0;
    UInt8 existingAttributes = 0;
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

    if (!BuildShortName(name, shortName)) {
      return false;
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

    UInt32 clusterLBA
      = _volume->_dataStartLBA + (newCluster - 2) * _volume->_sectorsPerCluster;
    UInt8 zeroSector[512] = {};

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

    _volume->WriteUInt16(dirSector, 26, static_cast<UInt16>(newCluster));
    _volume->WriteUInt32(dirSector, 28, 0);

    for (UInt32 i = 0; i < 11; ++i) {
      dirSector[32 + i] = dotDotName[i];
    }

    dirSector[32 + 11] = 0x10;

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

    if (
      !FindFreeSlot(
        parentCluster,
        parentIsRoot,
        entryLBA,
        entryOffset
      )
    ) {
      return false;
    }

    UInt8 entryBytes[32] = {};

    for (UInt32 i = 0; i < 11; ++i) {
      entryBytes[i] = shortName[i];
    }

    entryBytes[11] = 0x10;

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

    UInt32 existingCluster = 0;
    UInt8 existingAttributes = 0;
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

    if (!BuildShortName(name, shortName)) {
      return false;
    }

    UInt32 entryLBA = 0;
    UInt32 entryOffset = 0;

    if (
      !FindFreeSlot(
        parentCluster,
        parentIsRoot,
        entryLBA,
        entryOffset
      )
    ) {
      return false;
    }

    UInt8 entryBytes[32] = {};

    for (UInt32 i = 0; i < 11; ++i) {
      entryBytes[i] = shortName[i];
    }

    entryBytes[11] = 0x20;

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

    request.lba = lba;
    request.buffer = sector;

    return BlockDevice::Write(request) == 0;
  }

  bool Directory::GetEntryInfo(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    UInt8& outAttributes,
    UInt32& outSize
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

    outAttributes = record.attributes;
    outSize = record.sizeBytes;

    return true;
  }

}
