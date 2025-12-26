/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Volume.cpp
 * FAT12 volume metadata.
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevice.hpp>

#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12 {
  using BlockDevice = ABI::Devices::BlockDevice;
  using FileSystem = ABI::FileSystem;

  bool Volume::Load() {
    _valid = false;

    // ensure we have a floppy block device
    if (!GetFloppyInfo(_device)) {
      return false;
    }

    UInt8 bootSector[512] = {};

    // ensure we can read the boot sector
    if (!ReadBootSector(bootSector, sizeof(bootSector))) {
      return false;
    }

    UInt16 bytesPerSector = ReadUInt16(bootSector, 11);
    UInt8 sectorsPerCluster = bootSector[13];
    UInt16 reservedSectors = ReadUInt16(bootSector, 14);
    UInt8 fatCount = bootSector[16];
    UInt16 rootEntryCount = ReadUInt16(bootSector, 17);
    UInt16 totalSectors16 = ReadUInt16(bootSector, 19);
    UInt16 sectorsPerFAT = ReadUInt16(bootSector, 22);
    UInt32 totalSectors32 = ReadUInt32(bootSector, 32);
    UInt32 totalSectors = totalSectors16 != 0
      ? totalSectors16
      : totalSectors32;

    // validate essential parameters
    if (
      bytesPerSector == 0 ||
      sectorsPerCluster == 0 ||
      reservedSectors == 0 ||
      fatCount == 0 ||
      sectorsPerFAT == 0 ||
      totalSectors == 0
    ) {
      return false;
    }

    UInt32 rootDirBytes = static_cast<UInt32>(rootEntryCount) * 32;
    UInt32 rootDirSectors
      = (rootDirBytes + bytesPerSector - 1) / bytesPerSector;
    UInt32 fatStartLBA = reservedSectors;
    UInt32 rootDirStartLBA = fatStartLBA + fatCount * sectorsPerFAT;
    UInt32 dataStartLBA = rootDirStartLBA + rootDirSectors;
    UInt32 dataSectors = totalSectors - dataStartLBA;
    UInt32 clusterCount = dataSectors / sectorsPerCluster;

    _info = FileSystem::VolumeInfo {};
    _info.label[0] = 'A';
    _info.label[1] = '\0';
    _info.fsType = static_cast<UInt32>(FileSystem::Type::FAT12);
    _info.sectorSize = bytesPerSector;
    _info.sectorCount = totalSectors;
    _info.freeSectors = 0;

    _fatStartLBA = fatStartLBA;
    _fatSectors = sectorsPerFAT;
    _fatCount = fatCount;
    _rootDirectoryStartLBA = rootDirStartLBA;
    _rootDirectorySectors = rootDirSectors;
    _dataStartLBA = dataStartLBA;
    _sectorsPerCluster = sectorsPerCluster;
    _rootEntryCount = rootEntryCount;
    _clusterCount = clusterCount;
    _valid = true;
    _nextFreeCluster = 2;

    LoadFATCache();

    return true;
  }

  bool Volume::IsValid() const {
    return _valid;
  }

  const FileSystem::VolumeInfo& Volume::GetInfo() const {
    return _info;
  }

  FileSystem::VolumeHandle Volume::GetHandle() const {
    return _handle;
  }

  bool Volume::MatchesLabel(CString label) const {
    if (!_valid) {
      return false;
    }

    return MatchLabel(label, "A");
  }

  void Volume::FillEntry(FileSystem::VolumeEntry& entry) const {
    entry.label[0] = _info.label[0];
    entry.label[1] = _info.label[1];
    entry.fsType = _info.fsType;
  }

  UInt32 Volume::GetRootEntryCount() const {
    return _rootEntryCount;
  }

  bool Volume::ReadRootEntry(
    UInt32 index,
    FileSystem::DirectoryEntry& entry,
    bool& end
  ) {
    DirectoryRecord record {};

    if (!ReadRootRecord(index, record, end)) {
      return false;
    }

    return RecordToEntry(record, entry);
  }

  bool Volume::ReadDirectoryEntry(
    UInt32 startCluster,
    UInt32 index,
    FileSystem::DirectoryEntry& entry,
    bool& end
  ) {
    DirectoryRecord record {};

    if (!ReadDirectoryRecord(startCluster, index, record, end)) {
      return false;
    }

    return RecordToEntry(record, entry);
  }

  bool Volume::FindEntry(
    UInt32 startCluster,
    bool isRoot,
    CString name,
    UInt32& outCluster,
    UInt8& outAttributes,
    UInt32& outSize
  ) {
    if (!name || name[0] == '\0') {
      return false;
    }

    DirectoryRecord record {};
    bool end = false;
    UInt32 index = 0;

    for (;;) {
      bool ok = false;

      if (isRoot) {
        ok = ReadRootRecord(index, record, end);
      } else {
        ok = ReadDirectoryRecord(startCluster, index, record, end);
      }

      if (ok) {
        FileSystem::DirectoryEntry entry {};

        if (RecordToEntry(record, entry)) {
          if (MatchName(entry.name, name)) {
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

  bool Volume::ReadFile(
    UInt32 startCluster,
    UInt32 offset,
    UInt8* buffer,
    UInt32 length,
    UInt32& outRead,
    UInt32 fileSize
  ) {
    outRead = 0;

    if (!_valid || !buffer || length == 0) {
      return false;
    }

    if (offset >= fileSize) {
      return true;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 remaining = length;
    UInt32 maxReadable = fileSize - offset;

    if (remaining > maxReadable) {
      remaining = maxReadable;
    }

    if (startCluster < 2) {
      return true;
    }

    UInt32 clusterSize = bytesPerSector * _sectorsPerCluster;
    UInt32 skipClusters = offset / clusterSize;
    UInt32 clusterOffset = offset % clusterSize;
    UInt32 cluster = startCluster;

    for (UInt32 i = 0; i < skipClusters; ++i) {
      UInt32 nextCluster = 0;

      if (!ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        return true;
      }

      cluster = nextCluster;
    }

    UInt32 sectorOffset = clusterOffset / bytesPerSector;
    UInt32 byteOffset = clusterOffset % bytesPerSector;

    while (remaining > 0) {
      UInt8 sector[512] = {};
      UInt32 lba
        = _dataStartLBA
        + (cluster - 2) * _sectorsPerCluster
        + sectorOffset;

      BlockDevice::Request request {};

      request.deviceId = _device.id;
      request.lba = lba;
      request.count = 1;
      request.buffer = sector;

      if (BlockDevice::Read(request) != 0) {
        return false;
      }

      UInt32 copyStart = byteOffset;
      UInt32 copyBytes = bytesPerSector - copyStart;

      if (copyBytes > remaining) {
        copyBytes = remaining;
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        buffer[outRead + i] = sector[copyStart + i];
      }

      outRead += copyBytes;
      remaining -= copyBytes;

      if (remaining == 0) {
        break;
      }

      byteOffset = 0;
      sectorOffset++;

      if (sectorOffset >= _sectorsPerCluster) {
        UInt32 nextCluster = 0;

        if (!ReadFATEntry(cluster, nextCluster)) {
          return false;
        }

        if (IsEndOfChain(nextCluster)) {
          break;
        }

        cluster = nextCluster;
        sectorOffset = 0;
      }
    }

    return true;
  }

  bool Volume::WriteFileData(
    UInt32& startCluster,
    UInt32 offset,
    const UInt8* buffer,
    UInt32 length,
    UInt32& outWritten,
    UInt32 fileSize,
    UInt32& outSize
  ) {
    outWritten = 0;
    outSize = fileSize;

    if (!_valid || !buffer || length == 0) {
      return false;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 clusterSize = bytesPerSector * _sectorsPerCluster;
    UInt32 endOffset = offset + length;

    if (startCluster == 0) {
      UInt32 firstCluster = 0;

      if (!FindFreeCluster(firstCluster)) {
        return false;
      }

      if (!WriteFATEntry(firstCluster, 0xFFF)) {
        return false;
      }

      startCluster = firstCluster;
    }

    UInt32 clustersNeeded = (endOffset + clusterSize - 1) / clusterSize;
    UInt32 clusterCount = 0;
    UInt32 cluster = startCluster;
    UInt32 lastCluster = startCluster;

    for (;;) {
      ++clusterCount;

      UInt32 nextCluster = 0;

      if (!ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        lastCluster = cluster;
        break;
      }

      cluster = nextCluster;
    }

    while (clusterCount < clustersNeeded) {
      UInt32 newCluster = 0;

      if (!FindFreeCluster(newCluster)) {
        return false;
      }

      if (!WriteFATEntry(lastCluster, newCluster)) {
        return false;
      }

      if (!WriteFATEntry(newCluster, 0xFFF)) {
        return false;
      }

      lastCluster = newCluster;
      ++clusterCount;
    }

    UInt32 skipClusters = offset / clusterSize;
    UInt32 clusterOffset = offset % clusterSize;
    UInt32 currentCluster = startCluster;

    for (UInt32 i = 0; i < skipClusters; ++i) {
      UInt32 nextCluster = 0;

      if (!ReadFATEntry(currentCluster, nextCluster)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        return false;
      }

      currentCluster = nextCluster;
    }

    UInt32 sectorOffset = clusterOffset / bytesPerSector;
    UInt32 byteOffset = clusterOffset % bytesPerSector;
    UInt32 remaining = length;

    while (remaining > 0) {
      UInt8 sector[512] = {};
      UInt32 lba
        = _dataStartLBA
        + (currentCluster - 2) * _sectorsPerCluster
        + sectorOffset;

      BlockDevice::Request request {};

      request.deviceId = _device.id;
      request.lba = lba;
      request.count = 1;
      request.buffer = sector;

      if (BlockDevice::Read(request) != 0) {
        return false;
      }

      UInt32 copyStart = byteOffset;
      UInt32 copyBytes = bytesPerSector - copyStart;

      if (copyBytes > remaining) {
        copyBytes = remaining;
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        sector[copyStart + i] = buffer[outWritten + i];
      }

      request.lba = lba;
      request.buffer = sector;

      if (BlockDevice::Write(request) != 0) {
        return false;
      }

      outWritten += copyBytes;
      remaining -= copyBytes;

      if (remaining == 0) {
        break;
      }

      byteOffset = 0;
      sectorOffset++;

      if (sectorOffset >= _sectorsPerCluster) {
        UInt32 nextCluster = 0;

        if (!ReadFATEntry(currentCluster, nextCluster)) {
          return false;
        }

        if (IsEndOfChain(nextCluster)) {
          break;
        }

        currentCluster = nextCluster;
        sectorOffset = 0;
      }
    }

    if (endOffset > outSize) {
      outSize = endOffset;
    }

    return true;
  }

  bool Volume::GetEntryInfo(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    FileSystem::FileInfo& outInfo,
    UInt8& outAttributes
  ) {
    UInt32 cluster = 0;
    UInt32 sizeBytes = 0;

    if (
      !FindEntry(
        parentCluster,
        parentIsRoot,
        name,
        cluster,
        outAttributes,
        sizeBytes
      )
    ) {
      return false;
    }

    outInfo = FileSystem::FileInfo {};
    outInfo.sizeBytes = sizeBytes;
    outInfo.attributes = outAttributes;

    return true;
  }

  bool Volume::GetEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    DirectoryRecord record {};

    return FindEntryLocation(
      parentCluster,
      parentIsRoot,
      name,
      record,
      outLBA,
      outOffset
    );
  }

  bool Volume::UpdateEntry(
    UInt32 lba,
    UInt32 offset,
    UInt16 startCluster,
    UInt32 sizeBytes
  ) {
    return UpdateDirectoryEntry(lba, offset, startCluster, sizeBytes);
  }

  bool Volume::CreateDirectory(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    if (!_valid) {
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

    if (!FindFreeCluster(newCluster)) {
      return false;
    }

    if (!WriteFATEntry(newCluster, 0xFFF)) {
      return false;
    }

    UInt32 clusterLBA
      = _dataStartLBA + (newCluster - 2) * _sectorsPerCluster;
    UInt8 zeroSector[512] = {};

    for (UInt32 s = 0; s < _sectorsPerCluster; ++s) {
      BlockDevice::Request request {};

      request.deviceId = _device.id;
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

    WriteUInt16(dirSector, 26, static_cast<UInt16>(newCluster));
    WriteUInt32(dirSector, 28, 0);

    for (UInt32 i = 0; i < 11; ++i) {
      dirSector[32 + i] = dotDotName[i];
    }

    dirSector[32 + 11] = 0x10;

    WriteUInt16(
      dirSector,
      32 + 26,
      static_cast<UInt16>(parentIsRoot ? 0 : parentCluster)
    );
    WriteUInt32(dirSector, 32 + 28, 0);

    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = clusterLBA;
    request.count = 1;
    request.buffer = dirSector;

    if (BlockDevice::Write(request) != 0) {
      return false;
    }

    UInt32 entryLBA = 0;
    UInt32 entryOffset = 0;

    if (
      !FindFreeDirectorySlot(
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

    WriteUInt16(entryBytes, 26, static_cast<UInt16>(newCluster));
    WriteUInt32(entryBytes, 28, 0);

    return WriteDirectoryEntry(entryLBA, entryOffset, entryBytes);
  }

  bool Volume::CreateFile(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    if (!_valid) {
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
      !FindFreeDirectorySlot(
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

    WriteUInt16(entryBytes, 26, 0);
    WriteUInt32(entryBytes, 28, 0);

    return WriteDirectoryEntry(entryLBA, entryOffset, entryBytes);
  }

  bool Volume::ReadFATEntry(UInt32 cluster, UInt32& nextCluster) {
    if (!_valid) {
      return false;
    }

    if (ReadFATEntryCached(cluster, nextCluster)) {
      return true;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 fatOffset = cluster + (cluster / 2);
    UInt32 sectorOffset = fatOffset / bytesPerSector;
    UInt32 byteOffset = fatOffset % bytesPerSector;
    UInt32 fatLBA = _fatStartLBA + sectorOffset;

    UInt8 sector[512] = {};

    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = fatLBA;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    UInt16 value = 0;

    if (byteOffset == bytesPerSector - 1) {
      UInt8 nextSector[512] = {};

      request.lba = fatLBA + 1;
      request.buffer = nextSector;

      if (BlockDevice::Read(request) != 0) {
        return false;
      }

      value = static_cast<UInt16>(
        sector[byteOffset]
        | (static_cast<UInt16>(nextSector[0]) << 8)
      );
    } else {
      value = static_cast<UInt16>(
        sector[byteOffset]
        | (static_cast<UInt16>(sector[byteOffset + 1]) << 8)
      );
    }

    if ((cluster & 1u) != 0) {
      nextCluster = static_cast<UInt32>(value >> 4);
    } else {
      nextCluster = static_cast<UInt32>(value & 0x0FFF);
    }

    return true;
  }

  bool Volume::LoadFATCache() {
    _fatCached = false;
    _fatCacheBytes = 0;

    if (!_valid || _fatSectors == 0) {
      return false;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 fatBytes = _fatSectors * bytesPerSector;

    if (fatBytes > sizeof(_fatCache)) {
      return false;
    }

    for (UInt32 s = 0; s < _fatSectors; ++s) {
      BlockDevice::Request request {};

      request.deviceId = _device.id;
      request.lba = _fatStartLBA + s;
      request.count = 1;
      request.buffer = _fatCache + (s * bytesPerSector);

      if (BlockDevice::Read(request) != 0) {
        return false;
      }
    }

    _fatCacheBytes = fatBytes;
    _fatCached = true;

    return true;
  }

  bool Volume::ReadFATEntryCached(
    UInt32 cluster,
    UInt32& nextCluster
  ) const {
    if (!_fatCached || _fatCacheBytes == 0) {
      return false;
    }

    UInt32 fatOffset = cluster + (cluster / 2);

    if (fatOffset + 1 >= _fatCacheBytes) {
      return false;
    }

    UInt16 value = static_cast<UInt16>(
      _fatCache[fatOffset]
      | (static_cast<UInt16>(_fatCache[fatOffset + 1]) << 8)
    );

    if ((cluster & 1u) != 0) {
      nextCluster = static_cast<UInt32>(value >> 4);
    } else {
      nextCluster = static_cast<UInt32>(value & 0x0FFF);
    }

    return true;
  }

  bool Volume::IsEndOfChain(UInt32 value) {
    return value >= 0xFF8;
  }

  bool Volume::WriteFATEntry(UInt32 cluster, UInt32 value) {
    if (!_valid) {
      return false;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 fatOffset = cluster + (cluster / 2);
    UInt32 sectorOffset = fatOffset / bytesPerSector;
    UInt32 byteOffset = fatOffset % bytesPerSector;

    if (sectorOffset >= _fatSectors) {
      return false;
    }

    UInt16 packed = static_cast<UInt16>(value & 0x0FFF);

    if (_fatCached && fatOffset + 1 < _fatCacheBytes) {
      UInt16 existing = static_cast<UInt16>(
        _fatCache[fatOffset]
        | (static_cast<UInt16>(_fatCache[fatOffset + 1]) << 8)
      );

      if ((cluster & 1u) != 0) {
        existing = static_cast<UInt16>((existing & 0x000F) | (packed << 4));
      } else {
        existing = static_cast<UInt16>((existing & 0xF000) | packed);
      }

      _fatCache[fatOffset] = static_cast<UInt8>(existing & 0xFF);
      _fatCache[fatOffset + 1] = static_cast<UInt8>((existing >> 8) & 0xFF);
    }

    for (UInt32 fatIndex = 0; fatIndex < _fatCount; ++fatIndex) {
      UInt32 fatLBA = _fatStartLBA + fatIndex * _fatSectors + sectorOffset;
      UInt8 sector[512] = {};

      BlockDevice::Request request {};

      request.deviceId = _device.id;
      request.lba = fatLBA;
      request.count = 1;
      request.buffer = sector;

      if (BlockDevice::Read(request) != 0) {
        return false;
      }

      if (byteOffset == bytesPerSector - 1) {
        UInt8 nextSector[512] = {};

        request.lba = fatLBA + 1;
        request.buffer = nextSector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        UInt16 existing = static_cast<UInt16>(
          sector[byteOffset]
          | (static_cast<UInt16>(nextSector[0]) << 8)
        );

        if ((cluster & 1u) != 0) {
          existing = static_cast<UInt16>((existing & 0x000F) | (packed << 4));
        } else {
          existing = static_cast<UInt16>((existing & 0xF000) | packed);
        }

        sector[byteOffset] = static_cast<UInt8>(existing & 0xFF);
        nextSector[0] = static_cast<UInt8>((existing >> 8) & 0xFF);

        request.lba = fatLBA;
        request.buffer = sector;

        if (BlockDevice::Write(request) != 0) {
          return false;
        }

        request.lba = fatLBA + 1;
        request.buffer = nextSector;

        if (BlockDevice::Write(request) != 0) {
          return false;
        }
      } else {
        UInt16 existing = static_cast<UInt16>(
          sector[byteOffset]
          | (static_cast<UInt16>(sector[byteOffset + 1]) << 8)
        );

        if ((cluster & 1u) != 0) {
          existing = static_cast<UInt16>((existing & 0x000F) | (packed << 4));
        } else {
          existing = static_cast<UInt16>((existing & 0xF000) | packed);
        }

        sector[byteOffset] = static_cast<UInt8>(existing & 0xFF);
        sector[byteOffset + 1] = static_cast<UInt8>((existing >> 8) & 0xFF);

        request.lba = fatLBA;
        request.buffer = sector;

        if (BlockDevice::Write(request) != 0) {
          return false;
        }
      }
    }

    return true;
  }

  bool Volume::FindFreeCluster(UInt32& outCluster) {
    if (!_valid || _clusterCount == 0) {
      return false;
    }

    UInt32 maxCluster = _clusterCount + 1;
    UInt32 start = _nextFreeCluster;

    if (start < 2 || start > maxCluster) {
      start = 2;
    }

    for (UInt32 cluster = start; cluster <= maxCluster; ++cluster) {
      UInt32 nextCluster = 0;

      if (_fatCached) {
        if (!ReadFATEntryCached(cluster, nextCluster)) {
          return false;
        }
      } else {
        if (!ReadFATEntry(cluster, nextCluster)) {
          return false;
        }
      }

      if (nextCluster == 0x000) {
        outCluster = cluster;
        _nextFreeCluster = cluster + 1;

        return true;
      }
    }

    for (UInt32 cluster = 2; cluster < start; ++cluster) {
      UInt32 nextCluster = 0;

      if (_fatCached) {
        if (!ReadFATEntryCached(cluster, nextCluster)) {
          return false;
        }
      } else {
        if (!ReadFATEntry(cluster, nextCluster)) {
          return false;
        }
      }

      if (nextCluster == 0x000) {
        outCluster = cluster;
        _nextFreeCluster = cluster + 1;

        return true;
      }
    }

    return false;
  }

  bool Volume::ReadRootRecord(
    UInt32 index,
    DirectoryRecord& record,
    bool& end
  ) {
    end = false;

    if (!_valid) {
      return false;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    if (index >= _rootEntryCount) {
      end = true;

      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 sectorIndex = index / entriesPerSector;
    UInt32 entryIndex = index % entriesPerSector;

    // check for end of root directory
    if (sectorIndex >= _rootDirectorySectors) {
      end = true;

      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = _rootDirectoryStartLBA + sectorIndex;
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
    record.startCluster = ReadUInt16(base, 26);
    record.sizeBytes = ReadUInt32(base, 28);

    return true;
  }

  bool Volume::ReadBootSector(UInt8* buffer, UInt32 bufferBytes) {
    if (!buffer || bufferBytes < 512) {
      return false;
    }

    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = _bootSectorLBA;
    request.count = 1;
    request.buffer = buffer;

    return BlockDevice::Read(request) == 0;
  }

  bool Volume::GetFloppyInfo(BlockDevice::Info& outInfo) {
    UInt32 count = BlockDevice::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      BlockDevice::Info info {};

      // find the first floppy device to bind to this service
      if (BlockDevice::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type == BlockDevice::Type::Floppy) {
        outInfo = info;

        return true;
      }
    }

    return false;
  }

  UInt16 Volume::ReadUInt16(const UInt8* base, UInt32 offset) {
    return static_cast<UInt16>(
      base[offset] | (static_cast<UInt16>(base[offset + 1]) << 8)
    );
  }

  UInt32 Volume::ReadUInt32(const UInt8* base, UInt32 offset) {
    return static_cast<UInt32>(base[offset])
      | (static_cast<UInt32>(base[offset + 1]) << 8)
      | (static_cast<UInt32>(base[offset + 2]) << 16)
      | (static_cast<UInt32>(base[offset + 3]) << 24);
  }

  void Volume::WriteUInt16(UInt8* base, UInt32 offset, UInt16 value) {
    base[offset] = static_cast<UInt8>(value & 0xFF);
    base[offset + 1] = static_cast<UInt8>((value >> 8) & 0xFF);
  }

  void Volume::WriteUInt32(UInt8* base, UInt32 offset, UInt32 value) {
    base[offset] = static_cast<UInt8>(value & 0xFF);
    base[offset + 1] = static_cast<UInt8>((value >> 8) & 0xFF);
    base[offset + 2] = static_cast<UInt8>((value >> 16) & 0xFF);
    base[offset + 3] = static_cast<UInt8>((value >> 24) & 0xFF);
  }

  bool Volume::MatchLabel(CString label, CString expected) {
    if (!label || !expected) {
      return false;
    }

    UInt32 i = 0;

    while (label[i] != '\0' && expected[i] != '\0') {
      // case-insensitive compare for single-letter volume labels
      char a = label[i];
      char b = expected[i];

      if (a >= 'a' && a <= 'z') {
        a = static_cast<char>(a - 'a' + 'A');
      }

      if (b >= 'a' && b <= 'z') {
        b = static_cast<char>(b - 'a' + 'A');
      }

      if (a != b) {
        return false;
      }

      ++i;
    }

    if (label[i] == ':' && expected[i] == '\0' && label[i + 1] == '\0') {
      return true;
    }

    return label[i] == '\0' && expected[i] == '\0';
  }

  bool Volume::MatchName(CString left, CString right) {
    if (!left || !right) {
      return false;
    }

    UInt32 i = 0;

    while (left[i] != '\0' && right[i] != '\0') {
      char a = left[i];
      char b = right[i];

      if (a >= 'a' && a <= 'z') {
        a = static_cast<char>(a - 'a' + 'A');
      }

      if (b >= 'a' && b <= 'z') {
        b = static_cast<char>(b - 'a' + 'A');
      }

      if (a != b) {
        return false;
      }

      ++i;
    }

    return left[i] == '\0' && right[i] == '\0';
  }

  void Volume::FormatName(
    const UInt8* base,
    char* outName,
    UInt32 outBytes
  ) {
    if (!base || !outName || outBytes == 0) {
      return;
    }

    UInt32 nameLength = 0;
    UInt32 extLength = 0;

    for (UInt32 i = 0; i < 8; ++i) {
      if (base[i] == ' ') {
        break;
      }

      if (nameLength + 1 < outBytes) {
        outName[nameLength++] = static_cast<char>(base[i]);
      }
    }

    for (UInt32 i = 0; i < 3; ++i) {
      UInt8 c = base[8 + i];

      if (c == ' ') {
        break;
      }

      if (nameLength + extLength + 2 < outBytes) {
        outName[nameLength + 1 + extLength] = static_cast<char>(c);
        ++extLength;
      }
    }

    if (extLength > 0 && nameLength + 1 < outBytes) {
      outName[nameLength] = '.';
      outName[nameLength + 1 + extLength] = '\0';
    } else if (nameLength < outBytes) {
      outName[nameLength] = '\0';
    }
  }

  bool Volume::BuildShortName(CString name, UInt8* outName) {
    if (!name || !outName) {
      return false;
    }

    for (UInt32 i = 0; i < 11; ++i) {
      outName[i] = ' ';
    }

    UInt32 nameLen = 0;
    UInt32 extLen = 0;
    bool inExt = false;

    for (UInt32 i = 0; name[i] != '\0'; ++i) {
      char c = name[i];

      if (c == '.') {
        inExt = true;
        continue;
      }

      if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - 'a' + 'A');
      }

      if (inExt) {
        if (extLen >= 3) {
          return false;
        }

        outName[8 + extLen++] = static_cast<UInt8>(c);
      } else {
        if (nameLen >= 8) {
          return false;
        }

        outName[nameLen++] = static_cast<UInt8>(c);
      }
    }

    return nameLen > 0;
  }

  bool Volume::ReadDirectoryRecord(
    UInt32 startCluster,
    UInt32 index,
    DirectoryRecord& record,
    bool& end
  ) {
    end = false;

    if (!_valid || startCluster < 2) {
      return false;
    }

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;
    UInt32 entriesPerCluster = entriesPerSector * _sectorsPerCluster;
    UInt32 clusterIndex = index / entriesPerCluster;
    UInt32 entryOffset = index % entriesPerCluster;

    UInt32 cluster = startCluster;

    for (UInt32 i = 0; i < clusterIndex; ++i) {
      UInt32 nextCluster = 0;

      if (!ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        end = true;

        return false;
      }

      cluster = nextCluster;
    }

    UInt32 sectorInCluster = entryOffset / entriesPerSector;
    UInt32 entryIndex = entryOffset % entriesPerSector;
    UInt32 lba
      = _dataStartLBA + (cluster - 2) * _sectorsPerCluster + sectorInCluster;

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    const UInt8* base = sector + (entryIndex * 32);
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
    record.startCluster = ReadUInt16(base, 26);
    record.sizeBytes = ReadUInt32(base, 28);

    return true;
  }

  bool Volume::GetDirectoryEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    UInt32 index,
    UInt32& outLBA,
    UInt32& outOffset,
    bool& end
  ) {
    end = false;

    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;

    if (parentIsRoot) {
      if (index >= _rootEntryCount) {
        end = true;

        return false;
      }

      UInt32 sectorIndex = index / entriesPerSector;
      UInt32 entryIndex = index % entriesPerSector;

      if (sectorIndex >= _rootDirectorySectors) {
        end = true;

        return false;
      }

      outLBA = _rootDirectoryStartLBA + sectorIndex;
      outOffset = entryIndex * 32;

      return true;
    }

    if (parentCluster < 2) {
      return false;
    }

    UInt32 entriesPerCluster = entriesPerSector * _sectorsPerCluster;
    UInt32 clusterIndex = index / entriesPerCluster;
    UInt32 entryOffset = index % entriesPerCluster;
    UInt32 cluster = parentCluster;

    for (UInt32 i = 0; i < clusterIndex; ++i) {
      UInt32 nextCluster = 0;

      if (!ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        end = true;

        return false;
      }

      cluster = nextCluster;
    }

    UInt32 sectorInCluster = entryOffset / entriesPerSector;
    UInt32 entryIndex = entryOffset % entriesPerSector;

    outLBA
      = _dataStartLBA + (cluster - 2) * _sectorsPerCluster + sectorInCluster;
    outOffset = entryIndex * 32;

    return true;
  }

  bool Volume::FindFreeDirectorySlot(
    UInt32 parentCluster,
    bool parentIsRoot,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    UInt32 bytesPerSector = _info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 entriesPerSector = bytesPerSector / 32;

    if (parentIsRoot) {
      for (UInt32 s = 0; s < _rootDirectorySectors; ++s) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _device.id;
        request.lba = _rootDirectoryStartLBA + s;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        for (UInt32 e = 0; e < entriesPerSector; ++e) {
          UInt32 offset = e * 32;
          UInt8 first = sector[offset];

          if (first == 0x00 || first == 0xE5) {
            outLBA = _rootDirectoryStartLBA + s;
            outOffset = offset;

            return true;
          }
        }
      }

      return false;
    }

    UInt32 cluster = parentCluster;

    for (;;) {
      for (UInt32 s = 0; s < _sectorsPerCluster; ++s) {
        UInt8 sector[512] = {};
        BlockDevice::Request request {};

        request.deviceId = _device.id;
        request.lba
          = _dataStartLBA + (cluster - 2) * _sectorsPerCluster + s;
        request.count = 1;
        request.buffer = sector;

        if (BlockDevice::Read(request) != 0) {
          return false;
        }

        for (UInt32 e = 0; e < entriesPerSector; ++e) {
          UInt32 offset = e * 32;
          UInt8 first = sector[offset];

          if (first == 0x00 || first == 0xE5) {
            outLBA
              = _dataStartLBA + (cluster - 2) * _sectorsPerCluster + s;
            outOffset = offset;

            return true;
          }
        }
      }

      UInt32 nextCluster = 0;

      if (!ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        return false;
      }

      cluster = nextCluster;
    }
  }

  bool Volume::WriteDirectoryEntry(
    UInt32 lba,
    UInt32 offset,
    const UInt8* entryBytes
  ) {
    if (!entryBytes) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _device.id;
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

  bool Volume::FindEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    DirectoryRecord& record,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    if (!name || name[0] == '\0') {
      return false;
    }

    bool end = false;
    UInt32 index = 0;

    for (;;) {
      bool ok = false;

      if (parentIsRoot) {
        ok = ReadRootRecord(index, record, end);
      } else {
        ok = ReadDirectoryRecord(parentCluster, index, record, end);
      }

      if (ok) {
        FileSystem::DirectoryEntry entry {};

        if (RecordToEntry(record, entry)) {
          if (MatchName(entry.name, name)) {
            UInt32 lba = 0;
            UInt32 offset = 0;
            bool locEnd = false;

            if (GetDirectoryEntryLocation(
                parentCluster,
                parentIsRoot,
                index,
                lba,
                offset,
                locEnd
              )) {
              outLBA = lba;
              outOffset = offset;

              return true;
            }
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

  bool Volume::UpdateDirectoryEntry(
    UInt32 lba,
    UInt32 offset,
    UInt16 startCluster,
    UInt32 sizeBytes
  ) {
    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = lba;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevice::Read(request) != 0) {
      return false;
    }

    WriteUInt16(sector + offset, 26, startCluster);
    WriteUInt32(sector + offset, 28, sizeBytes);

    request.lba = lba;
    request.buffer = sector;

    return BlockDevice::Write(request) == 0;
  }

  bool Volume::MarkEntryDeleted(UInt32 lba, UInt32 offset) {
    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _device.id;
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

  bool Volume::RenameDirectoryEntry(
    UInt32 lba,
    UInt32 offset,
    const UInt8* shortName
  ) {
    if (!shortName) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _device.id;
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

  bool Volume::FreeClusterChain(UInt32 startCluster) {
    if (startCluster < 2) {
      return true;
    }

    UInt32 cluster = startCluster;

    for (;;) {
      UInt32 nextCluster = 0;

      if (!ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (!WriteFATEntry(cluster, 0x000)) {
        return false;
      }

      if (IsEndOfChain(nextCluster)) {
        break;
      }

      cluster = nextCluster;
    }

    return true;
  }

  bool Volume::IsDirectoryEmpty(UInt32 startCluster) {
    bool end = false;
    DirectoryRecord record {};
    UInt32 index = 0;

    for (;;) {
      if (ReadDirectoryRecord(startCluster, index, record, end)) {
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

  bool Volume::IsDotRecord(const DirectoryRecord& record) {
    if (record.name[0] != '.') {
      return false;
    }

    if (record.name[1] == ' ' || record.name[1] == '\0') {
      return true;
    }

    return record.name[1] == '.';
  }

  bool Volume::RemoveEntry(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    DirectoryRecord record {};
    UInt32 lba = 0;
    UInt32 offset = 0;

    if (!FindEntryLocation(
        parentCluster,
        parentIsRoot,
        name,
        record,
        lba,
        offset
      )) {
      return false;
    }

    if ((record.attributes & 0x10) != 0) {
      if (!IsDirectoryEmpty(record.startCluster)) {
        return false;
      }
    }

    if (!FreeClusterChain(record.startCluster)) {
      return false;
    }

    return MarkEntryDeleted(lba, offset);
  }

  bool Volume::RenameEntry(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    CString newName
  ) {
    DirectoryRecord record {};
    UInt32 lba = 0;
    UInt32 offset = 0;

    if (!FindEntryLocation(
        parentCluster,
        parentIsRoot,
        name,
        record,
        lba,
        offset
      )) {
      return false;
    }

    UInt32 existingCluster = 0;
    UInt8 existingAttributes = 0;
    UInt32 existingSize = 0;

    if (FindEntry(
        parentCluster,
        parentIsRoot,
        newName,
        existingCluster,
        existingAttributes,
        existingSize
      )) {
      return false;
    }

    UInt8 shortName[11] = {};

    if (!BuildShortName(newName, shortName)) {
      return false;
    }

    return RenameDirectoryEntry(lba, offset, shortName);
  }

  bool Volume::RecordToEntry(
    const DirectoryRecord& record,
    FileSystem::DirectoryEntry& entry
  ) {
    entry = FileSystem::DirectoryEntry {};
    entry.attributes = record.attributes;
    entry.sizeBytes = record.sizeBytes;

    FormatName(record.name, entry.name, FileSystem::maxDirectoryLength);

    return entry.name[0] != '\0';
  }
}
