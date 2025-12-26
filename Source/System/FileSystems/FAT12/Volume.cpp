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

  void Volume::Init() {
    _fat.Init(*this);
    _directory.Init(*this);
    _file.Init(*this);
  }

  bool Volume::Load() {
    // ensure helper pointers are wired
    Init();

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
    _freeClusters = 0;

    LoadFATCache();

    if (CountFreeClusters(_freeClusters)) {
      _info.freeSectors = _freeClusters * _sectorsPerCluster;
    }

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
    Directory::DirectoryRecord record {};

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
    Directory::DirectoryRecord record {};

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
    return _file.ReadFile(
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
    return _file.WriteFileData(
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
    UInt32 sizeBytes = 0;
    UInt8 attributes = 0;

    if (
      !_directory.GetEntryInfo(
        parentCluster,
        parentIsRoot,
        name,
        attributes,
        sizeBytes
      )
    ) {
      return false;
    }

    outInfo.sizeBytes = sizeBytes;
    outInfo.attributes = attributes;
    outAttributes = attributes;

    return true;
  }

  bool Volume::GetEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    return _directory.GetEntryLocation(
      parentCluster,
      parentIsRoot,
      name,
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
    return _directory.UpdateDirectoryEntry(
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
    return _fat.ReadFATEntry(cluster, nextCluster);
  }

  bool Volume::WriteFATEntry(UInt32 cluster, UInt32 value) {
    return _fat.WriteFATEntry(cluster, value);
  }

  bool Volume::FindFreeCluster(UInt32& outCluster) {
    return _fat.FindFreeCluster(outCluster);
  }

  bool Volume::CountFreeClusters(UInt32& outCount) {
    return _fat.CountFreeClusters(outCount);
  }

  bool Volume::LoadFATCache() {
    return _fat.LoadFATCache();
  }

  bool Volume::ReadFATEntryCached(UInt32 cluster, UInt32& nextCluster) const {
    return _fat.ReadFATEntryCached(cluster, nextCluster);
  }

  bool Volume::IsEndOfChain(UInt32 value) {
    return FAT::IsEndOfChain(value);
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

      if (BlockDevice::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type != BlockDevice::Type::Floppy) {
        continue;
      }

      outInfo = info;

      return true;
    }

    return false;
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

  bool Volume::ReadRootRecord(
    UInt32 index,
    Directory::DirectoryRecord& record,
    bool& end
  ) {
    return _directory.ReadRootRecord(index, record, end);
  }

  bool Volume::ReadDirectoryRecord(
    UInt32 startCluster,
    UInt32 index,
    Directory::DirectoryRecord& record,
    bool& end
  ) {
    return _directory.ReadDirectoryRecord(startCluster, index, record, end);
  }

  bool Volume::FindFreeDirectorySlot(
    UInt32 parentCluster,
    bool parentIsRoot,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    return _directory.FindFreeDirectorySlot(
      parentCluster,
      parentIsRoot,
      outLBA,
      outOffset
    );
  }

  bool Volume::WriteDirectoryEntry(
    UInt32 lba,
    UInt32 offset,
    const UInt8* entryBytes
  ) {
    return _directory.WriteDirectoryEntry(lba, offset, entryBytes);
  }

  bool Volume::FindEntryLocation(
    UInt32 parentCluster,
    bool parentIsRoot,
    CString name,
    Directory::DirectoryRecord& record,
    UInt32& outLBA,
    UInt32& outOffset
  ) {
    return _directory.FindEntryLocation(
      parentCluster,
      parentIsRoot,
      name,
      record,
      outLBA,
      outOffset
    );
  }

  bool Volume::UpdateDirectoryEntry(
    UInt32 lba,
    UInt32 offset,
    UInt16 startCluster,
    UInt32 sizeBytes
  ) {
    return _directory.UpdateDirectoryEntry(
      lba,
      offset,
      startCluster,
      sizeBytes
    );
  }

  bool Volume::MarkEntryDeleted(UInt32 lba, UInt32 offset) {
    return _directory.MarkEntryDeleted(lba, offset);
  }

  bool Volume::RenameDirectoryEntry(
    UInt32 lba,
    UInt32 offset,
    const UInt8* shortName
  ) {
    return _directory.RenameDirectoryEntry(lba, offset, shortName);
  }

  bool Volume::FreeClusterChain(UInt32 startCluster) {
    return _fat.FreeClusterChain(startCluster);
  }

  bool Volume::IsDirectoryEmpty(UInt32 startCluster) {
    return _directory.IsDirectoryEmpty(startCluster);
  }

  bool Volume::IsDotRecord(const Directory::DirectoryRecord& record) {
    return Directory::IsDotRecord(record);
  }

  bool Volume::RecordToEntry(
    const Directory::DirectoryRecord& record,
    FileSystem::DirectoryEntry& entry
  ) {
    return Directory::RecordToEntry(record, entry);
  }
}
