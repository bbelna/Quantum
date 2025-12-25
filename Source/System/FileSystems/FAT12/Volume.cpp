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

    _info = FileSystem::VolumeInfo {};
    _info.label[0] = 'A';
    _info.label[1] = '\0';
    _info.fsType = static_cast<UInt32>(FileSystem::Type::FAT12);
    _info.sectorSize = bytesPerSector;
    _info.sectorCount = totalSectors;
    _info.freeSectors = 0;

    _fatStartLBA = fatStartLBA;
    _fatSectors = sectorsPerFAT;
    _rootDirectoryStartLBA = rootDirStartLBA;
    _rootDirectorySectors = rootDirSectors;
    _dataStartLBA = dataStartLBA;
    _sectorsPerCluster = sectorsPerCluster;
    _rootEntryCount = rootEntryCount;
    _valid = true;

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

  bool Volume::ReadFATEntry(UInt32 cluster, UInt32& nextCluster) {
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

  bool Volume::IsEndOfChain(UInt32 value) {
    return value >= 0xFF8;
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
