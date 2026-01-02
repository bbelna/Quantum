/**
 * @file Services/FileSystems/FAT12/Volume.cpp
 * @brief FAT12 file system service volume handler.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/Devices/DeviceBroker.hpp>
#include <ABI/Handle.hpp>

#include "Volume.hpp"

namespace Quantum::Services::FileSystems::FAT12 {
  using ABI::Devices::BlockDevices;
  using ABI::FileSystem;

  void Volume::Initialize() {
    _fat.Initialize(*this);
    _directory.Initialize(*this);
    _file.Initialize(*this);
  }

  bool Volume::Load(const BlockDevices::Info& info) {
    // ensure helper pointers are wired
    Initialize();

    _valid = false;
    _device = info;
    _handle = static_cast<FileSystem::VolumeHandle>(info.id);

    if (_deviceHandle != 0) {
      ABI::Handle::Close(_deviceHandle);
      _deviceHandle = 0;
    }

    _deviceHandle = ABI::Devices::DeviceBroker::OpenBlockDevice(
      info.id,
      static_cast<UInt32>(BlockDevices::Right::Read) | static_cast<UInt32>(BlockDevices::Right::Write)
    );

    if (_deviceHandle == 0) {
      _deviceHandle = BlockDevices::Open(
        info.id,
        static_cast<UInt32>(BlockDevices::Right::Read) | static_cast<UInt32>(BlockDevices::Right::Write)
      );
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
      bytesPerSector == 0
      || sectorsPerCluster == 0
      || reservedSectors == 0
      || fatCount == 0
      || sectorsPerFAT == 0
      || totalSectors == 0
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

    BuildLabel(info, _info.label, sizeof(_info.label));

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
    _freeClusters = 0;

    LoadFATCache();

    if (CountFreeClusters(_freeClusters)) {
      _info.freeSectors = _freeClusters * _sectorsPerCluster;
    }

    return true;
  }

  bool Volume::Load() {
    BlockDevices::Info info {};

    if (!GetFloppyInfo(info)) {
      return false;
    }

    return Load(info);
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

  UInt32 Volume::GetDeviceToken() const {
    return _deviceHandle != 0 ? _deviceHandle : _device.id;
  }

  bool Volume::MatchesLabel(CString label) const {
    if (!_valid) {
      return false;
    }

    if (!label) {
      return false;
    }

    char normalized[FileSystem::maxLabelLength] = {};
    UInt32 idx = 0;

    while (label[idx] != '\0' && idx + 1 < sizeof(normalized)) {
      normalized[idx] = label[idx];
      ++idx;
    }

    normalized[idx] = '\0';

    if (idx > 0 && normalized[idx - 1] == ':') {
      normalized[idx - 1] = '\0';
    }

    return MatchLabel(normalized, _info.label);
  }

  void Volume::FillEntry(FileSystem::VolumeEntry& entry) const {
    for (UInt32 i = 0; i < sizeof(entry.label); ++i) {
      entry.label[i] = _info.label[i];
    }

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
    Directory::Record record {};

    if (!_directory.ReadRootRecord(index, record, end)) {
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
    Directory::Record record {};

    if (!_directory.ReadRecord(startCluster, index, record, end)) {
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
    return _directory.FindEntry(
      startCluster,
      isRoot,
      name,
      outCluster,
      outAttributes,
      outSize
    );
  }

  bool Volume::ReadFile(
    UInt32 startCluster,
    UInt32 offset,
    UInt8* buffer,
    UInt32 length,
    UInt32& outRead,
    UInt32 fileSize
  ) {
    return _file.Read(
      startCluster,
      offset,
      buffer,
      length,
      outRead,
      fileSize
    );
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
    return _file.Write(
      startCluster,
      offset,
      buffer,
      length,
      outWritten,
      fileSize,
      outSize
    );
  }

  bool Volume::GetEntryInfo(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    FileSystem::FileInfo& outInfo,
    UInt8& outAttributes
  ) {
    return _directory.GetEntryInfo(
      parentCluster,
      parentIsRoot,
      name,
      outInfo,
      outAttributes
    );
  }

  bool Volume::GetEntryInfoAt(
    UInt32 lba,
    UInt32 offset,
    FileSystem::FileInfo& outInfo,
    UInt8& outAttributes
  ) {
    Directory::Record record {};

    if (!_directory.ReadRecordAt(lba, offset, record)) {
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

  bool Volume::GetEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    Directory::Record record {};

    return _directory.FindEntryLocation(
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
    return _directory.UpdateEntry(
      lba,
      offset,
      startCluster,
      sizeBytes
    );
  }

  bool Volume::CreateDirectory(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    return _directory.CreateDirectory(parentCluster, parentIsRoot, name);
  }

  bool Volume::CreateFile(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    return _directory.CreateFile(parentCluster, parentIsRoot, name);
  }

  bool Volume::RemoveEntry(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name
  ) {
    return _directory.RemoveEntry(parentCluster, parentIsRoot, name);
  }

  bool Volume::RenameEntry(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    CString newName
  ) {
    return _directory.RenameEntry(parentCluster, parentIsRoot, name, newName);
  }

  bool Volume::ReadFATEntry(UInt32 cluster, UInt32& nextCluster) {
    return _fat.ReadEntry(cluster, nextCluster);
  }

  bool Volume::WriteFATEntry(UInt32 cluster, UInt32 value) {
    return _fat.WriteEntry(cluster, value);
  }

  bool Volume::FindFreeCluster(UInt32& outCluster) {
    return _fat.FindFreeCluster(outCluster);
  }

  bool Volume::CountFreeClusters(UInt32& outCount) {
    return _fat.CountFreeClusters(outCount);
  }

  bool Volume::LoadFATCache() {
    return _fat.LoadCache();
  }

  bool Volume::ReadFATEntryCached(UInt32 cluster, UInt32& nextCluster) const {
    return _fat.ReadEntryCached(cluster, nextCluster);
  }

  bool Volume::IsEndOfChain(UInt32 value) {
    return FAT::IsEndOfChain(value);
  }

  bool Volume::ReadBootSector(UInt8* buffer, UInt32 bufferBytes) {
    if (!buffer || bufferBytes < 512) {
      return false;
    }

    BlockDevices::Request request {};

    request.deviceId = GetDeviceToken();
    request.lba = _bootSectorLBA;
    request.count = 1;
    request.buffer = buffer;

    return BlockDevices::Read(request) == 0;
  }

  bool Volume::GetFloppyInfo(BlockDevices::Info& outInfo) {
    UInt32 count = BlockDevices::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      BlockDevices::Info info {};

      if (BlockDevices::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type != BlockDevices::Type::Floppy) {
        continue;
      }

      outInfo = info;

      return true;
    }

    return false;
  }

  void Volume::BuildLabel(
    const BlockDevices::Info& info,
    char* outLabel,
    UInt32 labelBytes
  ) {
    if (!outLabel || labelBytes == 0) {
      return;
    }

    for (UInt32 i = 0; i < labelBytes; ++i) {
      outLabel[i] = '\0';
    }

    char label = '?';

    if (info.type == BlockDevices::Type::Floppy && info.deviceIndex < 26) {
      label = static_cast<char>('A' + info.deviceIndex);
    }

    outLabel[0] = label;

    if (labelBytes > 1) {
      outLabel[1] = '\0';
    }
  }

  UInt16 Volume::ReadUInt16(const UInt8* base, UInt32 offset) {
    return static_cast<UInt16>(
      base[offset]
      | (static_cast<UInt16>(base[offset + 1]) << 8)
    );
  }

  UInt32 Volume::ReadUInt32(const UInt8* base, UInt32 offset) {
    return static_cast<UInt32>(
      base[offset]
      | (static_cast<UInt32>(base[offset + 1]) << 8)
      | (static_cast<UInt32>(base[offset + 2]) << 16)
      | (static_cast<UInt32>(base[offset + 3]) << 24)
    );
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

    UInt32 index = 0;

    while (label[index] != '\0' && expected[index] != '\0') {
      char left = label[index];
      char right = expected[index];

      if (left >= 'a' && left <= 'z') {
        left = static_cast<char>(left - 'a' + 'A');
      }

      if (right >= 'a' && right <= 'z') {
        right = static_cast<char>(right - 'a' + 'A');
      }

      if (left != right) {
        return false;
      }

      ++index;
    }

    return label[index] == '\0' && expected[index] == '\0';
  }

  bool Volume::MatchName(CString left, CString right) {
    if (!left || !right) {
      return false;
    }

    UInt32 index = 0;

    while (left[index] != '\0' && right[index] != '\0') {
      char a = left[index];
      char b = right[index];

      if (a >= 'a' && a <= 'z') {
        a = static_cast<char>(a - 'a' + 'A');
      }

      if (b >= 'a' && b <= 'z') {
        b = static_cast<char>(b - 'a' + 'A');
      }

      if (a != b) {
        return false;
      }

      ++index;
    }

    return left[index] == '\0' && right[index] == '\0';
  }

  bool Volume::FreeClusterChain(UInt32 startCluster) {
    return _fat.FreeClusterChain(startCluster);
  }

  bool Volume::IsDirectoryEmpty(UInt32 startCluster) {
    return _directory.IsEmpty(startCluster);
  }

  bool Volume::IsDotRecord(const Directory::Record& record) {
    return Directory::IsDotRecord(record);
  }

  bool Volume::RecordToEntry(
    const Directory::Record& record,
    FileSystem::DirectoryEntry& entry
  ) {
    return Directory::RecordToEntry(record, entry);
  }
}

