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

    if (!GetFloppyInfo(_device)) {
      return false;
    }

    UInt8 bootSector[512] = {};

    if (!ReadBootSector(bootSector, sizeof(bootSector))) {
      return false;
    }

    UInt16 bytesPerSector = ReadUInt16(bootSector, 11);
    UInt8 sectorsPerCluster = bootSector[13];
    UInt16 reservedSectors = ReadUInt16(bootSector, 14);
    UInt8 fatCount = bootSector[16];
    UInt16 rootEntryCount = ReadUInt16(bootSector, 17);
    UInt16 totalSectors16 = ReadUInt16(bootSector, 19);
    UInt16 sectorsPerFat = ReadUInt16(bootSector, 22);
    UInt32 totalSectors32 = ReadUInt32(bootSector, 32);
    UInt32 totalSectors = totalSectors16 != 0
      ? totalSectors16
      : totalSectors32;

    if (
      bytesPerSector == 0 ||
      sectorsPerCluster == 0 ||
      reservedSectors == 0 ||
      fatCount == 0 ||
      sectorsPerFat == 0 ||
      totalSectors == 0
    ) {
      return false;
    }

    UInt32 rootDirBytes = static_cast<UInt32>(rootEntryCount) * 32;
    UInt32 rootDirSectors
      = (rootDirBytes + bytesPerSector - 1) / bytesPerSector;
    UInt32 fatStartLba = reservedSectors;
    UInt32 rootDirStartLba = fatStartLba + fatCount * sectorsPerFat;
    UInt32 dataStartLba = rootDirStartLba + rootDirSectors;

    _info = FileSystem::VolumeInfo {};
    _info.label[0] = 'A';
    _info.label[1] = '\0';
    _info.fsType = static_cast<UInt32>(FileSystem::Type::FAT12);
    _info.sectorSize = bytesPerSector;
    _info.sectorCount = totalSectors;
    _info.freeSectors = 0;

    _fatStartLBA = fatStartLba;
    _fatSectors = sectorsPerFat;
    _rootDirectoryStartLBA = rootDirStartLba;
    _rootDirectorySectors = rootDirSectors;
    _dataStartLBA = dataStartLba;
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

    entry = FileSystem::DirectoryEntry {};
    entry.attributes = attributes;
    entry.sizeBytes = ReadUInt32(base, 28);

    FormatName(base, entry.name, FileSystem::maxDirectoryLength);

    return entry.name[0] != '\0';
  }

  bool Volume::ReadBootSector(UInt8* buffer, UInt32 bufferBytes) {
    if (!buffer || bufferBytes < 512) {
      return false;
    }

    BlockDevice::Request request {};

    request.deviceId = _device.id;
    request.lba = _bootSectorLba;
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
}
