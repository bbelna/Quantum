/**
 * @file FileSystems/FAT12/Volume.cpp
 * @brief Implements the FAT12 volume driver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/Byte.hpp>
#include <Quantum/Core/CString.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Kernel/ABI/Memory.hpp>
#include <Quantum/Memory.hpp>
#include <Quantum/Servers/Storage/ABI.hpp>

#include "Volume.hpp"

using namespace Quantum::Kernel;
using namespace Quantum::Kernel::ABI;

namespace FSABI = ::Quantum::Servers::FileSystem::ABI;
namespace KernelLog = Quantum::Kernel::ABI::Log;

namespace Quantum::FileSystems::FAT12 {
  Volume::~Volume() {
    _closeStorageHandles();
    _destroyPersistentBuffer();

    if (_fat) {
      FreeBlock(reinterpret_cast<UIntPtr>(_fat));

      _fat = nullptr;
    }
  }

  bool Volume::Mount(UInt32 deviceID) {
    _deviceID = deviceID;

    // read the BPB from sector 0
    UInt8 bpb[512];

    if (!_readSectors(0, 1, bpb)) {
      KernelLog::Write(Core::LogLevel::Error, "Failed to read BPB (sector 0)");

      return false;
    }

    // validate boot signature (0x55AA at offsets 510-511)
    if (bpb[510] != 0x55 || bpb[511] != 0xAA) {
      KernelLog::Write(
        Core::LogLevel::Error,
        "Bad boot signature (%u %u)",
        bpb[510], bpb[511]
      );

      return false;
    }

    // parse BPB fields (all values are little-endian)
    _bytesPerSector = static_cast<UInt32>(*reinterpret_cast<UInt16*>(&bpb[11]));
    _sectorsPerCluster = bpb[13];

    UInt32 reservedSectors = static_cast<UInt32>(
      *reinterpret_cast<UInt16*>(&bpb[14])
    );
    UInt32 numFATs = bpb[16];
    UInt32 rootEntryCount  = static_cast<UInt32>(
      *reinterpret_cast<UInt16*>(&bpb[17])
    );

    _fatSizeSectors = static_cast<UInt32>(*reinterpret_cast<UInt16*>(&bpb[22]));

    // total sectors: BPB offset 19 (16-bit) or offset 32 (32-bit fallback)
    UInt16 totalSectors16 = *reinterpret_cast<UInt16*>(&bpb[19]);

    _totalSectors = totalSectors16 != 0
      ? static_cast<UInt32>(totalSectors16)
      : *reinterpret_cast<UInt32*>(&bpb[32]);

    KernelLog::Write(
      Core::LogLevel::Trace,
      "Mounting device ID %u (BytesPerSector %u SectorsPerCluster %u "
      "NumberOfFATs %u FATSize %u RootEntryCount %u)",
      deviceID,
      _bytesPerSector,
      _sectorsPerCluster,
      numFATs,
      _fatSizeSectors,
      rootEntryCount
    );

    // basic sanity checks
    if (
      numFATs == 0 ||
      _bytesPerSector == 0 ||
      _sectorsPerCluster == 0 ||
      _fatSizeSectors == 0
    ) {
      KernelLog::Write(Core::LogLevel::Error, "BPB sanity check failed");

      return false;
    }

    _fatStart = reservedSectors;
    _rootDirectoryStart = _fatStart + numFATs * _fatSizeSectors;
    _rootDirectorySectors = (rootEntryCount * 32 + _bytesPerSector - 1)
                          / _bytesPerSector;
    _dataStart = _rootDirectoryStart + _rootDirectorySectors;
    _fatSize = _fatSizeSectors * _bytesPerSector;

    UIntPtr fatVA = Memory::ABI::Allocate(_fatSize);

    if (fatVA == 0) {
      KernelLog::Write(Core::LogLevel::Error, "FAT allocation failed");

      return false;
    }

    _fat = reinterpret_cast<UInt8*>(fatVA);

    if (!_readSectors(_fatStart, _fatSizeSectors, _fat)) {
      KernelLog::Write(Core::LogLevel::Error, "Failed to read FAT sectors");

      FreeBlock(fatVA);

      _fat = nullptr;

      return false;
    }

    _mounted = true;

    _extractLabel(bpb);

    return true;
  }

  UInt32 Volume::GetFreeBytes() const {
    if (!_mounted || !_fat) return 0;

    // total data clusters = (totalSectors - dataStart) / sectorsPerCluster
    UInt32 dataSectors = _totalSectors - _dataStart;
    UInt32 totalClusters = dataSectors / _sectorsPerCluster;
    UInt32 freeClusters = 0;

    // clusters are numbered starting at 2; entries 0 and 1 are reserved
    for (UInt32 cluster = 2; cluster < totalClusters + 2; ++cluster) {
      UInt32 fatValue = _nextCluster(static_cast<UInt16>(cluster));

      if (fatValue == 0) {
        ++freeClusters;
      }
    }

    return freeClusters * _sectorsPerCluster * _bytesPerSector;
  }

  void Volume::_extractLabel(const UInt8* bpb) {
    // seed from BPB offset 43 (11 bytes, space-padded) when extended BPB
    // signature 0x29 is present at offset 38
    int n = 0;

    if (bpb[38] == 0x29) {
      for (int i = 0; i < 11 && bpb[43 + i] != ' '; i++)
        _label[n++] = static_cast<char>(bpb[43 + i]);
    }

    _label[n] = '\0';

    // scan the root directory for a volume label entry (VolumeLabel = 0x08)
    // this takes precedence over the BPB field
    UInt8 sectorBuf[512];
    UInt32 entriesPerSector = _bytesPerSector / 32;

    for (UInt32 s = 0; s < _rootDirectorySectors; s++) {
      if (!_readSectors(_rootDirectoryStart + s, 1, sectorBuf)) break;

      const auto* raw = reinterpret_cast<const RawDirectoryEntry*>(sectorBuf);

      for (UInt32 i = 0; i < entriesPerSector; i++) {
        const RawDirectoryEntry& e = raw[i];

        if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryEnd) return;
        if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryDeleted) continue;
        if (e.Attributes != DirectoryAttribute::VolumeLabel) continue;

        // volume label: 8-char name + 3-char ext, no dot separator
        n = 0;

        for (int j = 0; j < 8 && e.Name[j] != ' '; j++)
          _label[n++] = e.Name[j];
        for (int j = 0; j < 3 && e.Extension[j]  != ' '; j++)
          _label[n++] = e.Extension[j];

        _label[n] = '\0';

        return;
      }
    }

    // fall back if neither source had a label
    if (_label[0] == '\0') Core::CString::Copy("VOLUME", _label, sizeof(_label));
  }

  bool Volume::FindEntry(
    const char* relativePath,
    ResolvedEntry* outEntry
  ) {
    if (!_mounted || !outEntry) return false;

    // empty path -> virtual root directory entry
    if (!relativePath || relativePath[0] == '\0') {
      outEntry->Valid = true;
      outEntry->IsDirectory = true;
      outEntry->FirstCluster = 0;
      outEntry->FileSize = 0;

      Core::CString::Copy("/", outEntry->Name, FSABI::FileSystemMaxFileNameLength);

      return true;
    }

    // skip leading slash or backslash
    const char* p = relativePath;

    if (*p == '/' || *p == '\\') p++;

    if (*p == '\0') {
      // path was just "/", treat as root
      outEntry->Valid = true;
      outEntry->IsDirectory = true;
      outEntry->FirstCluster = 0;
      outEntry->FileSize = 0;

      Core::CString::Copy("/", outEntry->Name, FSABI::FileSystemMaxFileNameLength);

      return true;
    }

    ResolvedEntry current;

    current.IsDirectory = true;
    current.FirstCluster = 0; // start at root (0 = root dir, not real cluster)
    current.Valid = true;

    while (*p != '\0') {
      // extract the next path component
      char component[FSABI::FileSystemMaxFileNameLength];
      UInt32 length = 0;

      while (p[length] != '\0' && p[length] != '/' && p[length] != '\\')
        length++;

      if (length == 0) {
        // consecutive slashes or trailing slash, skip
        p++;

        continue;
      }

      if (length >= FSABI::FileSystemMaxFileNameLength) return false;

      for (UInt32 i = 0; i < length; i++) component[i] = p[i];

      component[length] = '\0';
      p += length;

      if (*p == '/' || *p == '\\') p++;

      // must be descending into a directory
      if (!current.IsDirectory) return false;

      bool found;

      if (current.FirstCluster == 0) {
        found = _findRootEntry(component, outEntry);
      } else {
        found = _findSubdirectoryEntry(
          current.FirstCluster,
          component,
          outEntry
        );
      }

      if (!found) return false;

      current = *outEntry;
    }

    *outEntry = current;

    return outEntry->Valid;
  }

  bool Volume::ReadFile(
    const ResolvedEntry& entry,
    UInt32 offset,
    UInt32 size,
    void* buffer,
    UInt32& bytesRead
  ) {
    bytesRead = 0;

    if (!_mounted || entry.IsDirectory || !buffer) return false;
    if (entry.FileSize == 0) return true;  // empty file
    if (offset >= entry.FileSize) return true;  // past EOF
    if (offset + size > entry.FileSize) size = entry.FileSize - offset;
    if (size == 0) return true;

    UInt32 clusterSize = _sectorsPerCluster * _bytesPerSector;
    UInt8* dst = static_cast<UInt8*>(buffer);
    UInt32 remaining = size;

    // navigate to the cluster that contains 'offset'
    UInt16 cluster = entry.FirstCluster;
    UInt32 clusterIdx = offset / clusterSize;
    UInt32 intraOffset = offset % clusterSize;

    for (UInt32 i = 0; i < clusterIdx; i++) {
      if (cluster >= ClusterEOF) return false;  // premature EOF

      cluster = _nextCluster(cluster);
    }

    // allocate a cluster-sized buffer from the heap since cluster sizes
    // can exceed safe stack limits (e.g. 16 KB for 32 sectors/cluster)
    UIntPtr clusterBufVA = AllocateBlock(clusterSize);

    if (clusterBufVA == 0) return false;

    UInt8* clusterBuf = reinterpret_cast<UInt8*>(clusterBufVA);

    while (remaining > 0 && cluster >= 2 && cluster < ClusterEOF) {
      if (!_readSectors(_clusterToLBA(cluster), _sectorsPerCluster, clusterBuf)) {
        FreeBlock(clusterBufVA);

        return false;
      }

      UInt32 available = clusterSize - intraOffset;
      UInt32 toCopy = (remaining < available)
        ? remaining
        : available;

      Core::Byte::Copy(dst, clusterBuf + intraOffset, toCopy);

      dst += toCopy;
      bytesRead += toCopy;
      remaining -= toCopy;
      intraOffset = 0; // only first cluster may have an intra-cluster offset
      cluster = _nextCluster(cluster);
    }

    FreeBlock(clusterBufVA);

    return true;
  }

  bool Volume::ListDirectory(
    const char* relativePath,
    Servers::FileSystem::ABI::FileSystemDirectory* entries,
    UInt32 maxEntries,
    UInt32& outCount
  ) {
    outCount = 0;

    if (!_mounted || !entries) return false;

    // resolve to a directory entry
    ResolvedEntry dir;

    if (!FindEntry(relativePath, &dir)) return false;
    if (!dir.IsDirectory) return false;
    if (dir.FirstCluster == 0)
      return _listRootDirectory(entries, maxEntries, outCount);

    return _listSubdirectory(dir.FirstCluster, entries, maxEntries, outCount);
  }

  Volume::CachedSector* Volume::_cacheLookup(UInt64 lba) {
    for (UInt32 i = 0; i < SectorCacheSize; i++) {
      if (_sectorCache[i].Valid && _sectorCache[i].LBA == lba)
        return &_sectorCache[i];
    }

    return nullptr;
  }

  Volume::CachedSector* Volume::_cacheEvict() {
    CachedSector* slot = &_sectorCache[_cacheClockHand];
    _cacheClockHand = (_cacheClockHand + 1) % SectorCacheSize;
    slot->Valid = false;
    return slot;
  }

  bool Volume::_ensurePersistentBuffer(UInt32 sizeInBytes) {
    if (_persistentBufferID != 0 && _persistentBufferSize >= sizeInBytes)
      return true;

    _destroyPersistentBuffer();

    _persistentBufferID = Memory::ABI::CreateShared(sizeInBytes);

    if (_persistentBufferID == 0) return false;

    _persistentBufferVA = Memory::ABI::AttachShared(_persistentBufferID);

    if (_persistentBufferVA == 0) {
      _persistentBufferID = 0;
      return false;
    }

    _persistentBufferSize = sizeInBytes;
    return true;
  }

  bool Volume::_openStorageHandles() {
    using namespace Kernel;
    namespace SABI = Servers::Storage::ABI;

    _storageSendHandle = Kernel::ABI::IPC::Open(
      SABI::PortID,
      IPCPortRights::Send
    );

    if (_storageSendHandle == static_cast<IPCPortResourceID>(-1))
      return false;

    _storageReplyHandle = Kernel::ABI::IPC::Open(
      static_cast<IPCPortID>(-1),
      IPCPortRights::Manage | IPCPortRights::Receive,
      &_storageReplyPortID
    );

    if (_storageReplyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(_storageSendHandle);
      _storageSendHandle = static_cast<IPCPortResourceID>(-1);
      return false;
    }

    return true;
  }

  void Volume::_closeStorageHandles() {
    if (_storageReplyHandle != static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(_storageReplyHandle);

      _storageReplyHandle = static_cast<IPCPortResourceID>(-1);
    }

    if (_storageSendHandle != static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(_storageSendHandle);

      _storageSendHandle = static_cast<IPCPortResourceID>(-1);
    }
  }

  void Volume::_destroyPersistentBuffer() {
    if (_persistentBufferVA != 0) {
      Memory::ABI::DetachShared(_persistentBufferVA);

      _persistentBufferVA = 0;
    }

    _persistentBufferID = 0;
    _persistentBufferSize = 0;
  }

  bool Volume::_readSectors(UInt64 lba, UInt32 count, void* buffer) {
    UInt8* dst = static_cast<UInt8*>(buffer);

    if (count == 1) {
      CachedSector* cached = _cacheLookup(lba);

      if (cached) {
        Core::Byte::Copy(dst, cached->Data, _bytesPerSector);
        return true;
      }
    }

    UInt32 size = count * _bytesPerSector;

    if (!_ensurePersistentBuffer(size)) {
      KernelLog::Write(
        Core::LogLevel::Error,
        "Persistent buffer failed (LBA %u, Count %u, Size %u)",
        static_cast<UInt32>(lba), count, size
      );

      return false;
    }

    namespace SABI = Servers::Storage::ABI;

    if (_storageSendHandle == static_cast<Kernel::IPC::IPCPortResourceID>(-1)) {
      if (!_openStorageHandles()) return false;
    }

    SABI::ReadRequest request;
    request.ABIVersion  = SABI::Version;
    request.Operation   = SABI::StorageOperation::Read;
    request.ReplyPortID = _storageReplyPortID;
    request.DeviceID    = _deviceID;
    request.LBA         = lba;
    request.SectorCount = count;
    request.BufferID    = _persistentBufferID;

    Kernel::ABI::IPC::Send(
      _storageSendHandle, &request, sizeof(request)
    );

    IPCMessage* reply = Kernel::ABI::IPC::Receive(_storageReplyHandle);

    if (!reply) {
      KernelLog::Write(
        Core::LogLevel::Error,
        "Storage::Read IPC failed (LBA %u, Count %u)",
        static_cast<UInt32>(lba), count
      );

      return false;
    }

    bool success = false;

    if (reply->PayloadSizeInBytes >= sizeof(SABI::ReadResult)) {
      auto* result = static_cast<const SABI::ReadResult*>(reply->Payload);
      success = result->Success;
    }

    free(reply);

    if (!success) {
      KernelLog::Write(
        Core::LogLevel::Error,
        "Storage::Read returned failure (LBA %u, Count %u)",
        static_cast<UInt32>(lba), count
      );

      return false;
    }

    const UInt8* src = reinterpret_cast<const UInt8*>(
      _persistentBufferVA
    );

    Core::Byte::Copy(dst, src, size);

    for (UInt32 i = 0; i < count; i++) {
      UInt64 sectorLBA = lba + i;
      CachedSector* slot = _cacheLookup(sectorLBA);

      if (!slot) slot = _cacheEvict();

      slot->LBA = sectorLBA;
      slot->Valid = true;
      Core::Byte::Copy(slot->Data, src + i * _bytesPerSector, _bytesPerSector);
    }

    return true;
  }

  UInt16 Volume::_nextCluster(UInt16 cluster) const {
    if (!_fat || cluster < 2) return 0xFFFF;

    // each entry is 12 bits; two adjacent entries share 3 bytes
    UInt32 byteOffset = (cluster * 3) / 2;

    if (byteOffset + 1 >= _fatSize) return 0xFFFF;

    UInt16 word = static_cast<UInt16>(_fat[byteOffset]) |
                  (static_cast<UInt16>(_fat[byteOffset + 1]) << 8);

    return (cluster & 1) ? (word >> 4) : (word & 0x0FFF);
  }

  UInt64 Volume::_clusterToLBA(UInt16 cluster) const {
    return static_cast<UInt64>(_dataStart) +
           static_cast<UInt64>(cluster - 2) * _sectorsPerCluster;
  }

  bool Volume::_findRootEntry(
    const char* name,
    ResolvedEntry* outEntry
  ) {
    UInt8 sectorBuf[512];
    UInt32 entriesPerSector = _bytesPerSector / 32;
    char lfnBuf[LFNMaxLength + 1] = {};

    for (UInt32 s = 0; s < _rootDirectorySectors; s++) {
      if (!_readSectors(_rootDirectoryStart + s, 1, sectorBuf)) return false;

      RawDirectoryEntry* raw = reinterpret_cast<RawDirectoryEntry*>(sectorBuf);

      for (UInt32 i = 0; i < entriesPerSector; i++) {
        RawDirectoryEntry& e = raw[i];

        if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryEnd) {
          return false;
        } else if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryDeleted) {
          lfnBuf[0] = '\0';

          continue;
        } else if (e.Attributes == DirectoryAttribute::LongFileName) {
          _accumulateLFN(
            lfnBuf,
            reinterpret_cast<const RawLFNDirectoryEntry&>(e)
          );

          continue;
        } else if (
          ::Quantum::Core::Enum::HasAnyFlag(static_cast<DirectoryAttribute>(e.Attributes), DirectoryAttribute::VolumeLabel)
        ) {
          lfnBuf[0] = '\0';

          continue;
        }

        // try LFN name first, then fall back to 8.3
        bool matched = false;

        if (lfnBuf[0] != '\0') matched = _nameEquals(lfnBuf, name);

        if (!matched) {
          char shortName[FSABI::FileSystemMaxFileNameLength];

          _formatEntryName(e, shortName);

          matched = _nameEquals(shortName, name);
        }

        if (matched) {
          _fillResolved(e, outEntry, lfnBuf[0] != '\0' ? lfnBuf : nullptr);

          lfnBuf[0] = '\0';

          return true;
        }

        lfnBuf[0] = '\0';
      }
    }

    return false;
  }

  bool Volume::_findSubdirectoryEntry(
    UInt16 startCluster,
    const char* name,
    ResolvedEntry* outEntry
  ) {
    UInt8 sectorBuf[512];
    UInt32 entriesPerSector = _bytesPerSector / 32;
    UInt16 cluster = startCluster;
    char lfnBuf[LFNMaxLength + 1] = {};

    while (cluster >= 2 && cluster < ClusterEOF) {
      UInt64 lba = _clusterToLBA(cluster);

      for (UInt32 s = 0; s < _sectorsPerCluster; s++) {
        if (!_readSectors(lba + s, 1, sectorBuf)) return false;

        RawDirectoryEntry* raw = reinterpret_cast<RawDirectoryEntry*>(
          sectorBuf
        );

        for (UInt32 i = 0; i < entriesPerSector; i++) {
          RawDirectoryEntry& e = raw[i];

          if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryEnd) {
            return false;
          } else if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryDeleted) {
            lfnBuf[0] = '\0';

            continue;
          } else if (e.Attributes == DirectoryAttribute::LongFileName) {
            _accumulateLFN(
              lfnBuf,
              reinterpret_cast<const RawLFNDirectoryEntry&>(e)
            );

            continue;
          } else if (
            ::Quantum::Core::Enum::HasAnyFlag(static_cast<DirectoryAttribute>(e.Attributes), DirectoryAttribute::VolumeLabel)
          ) {
            lfnBuf[0] = '\0';

            continue;
          }

          // skip . and .. entries during name search
          if (e.Name[0] == '.') {
            lfnBuf[0] = '\0';

            continue;
          }

          bool matched = false;

          if (lfnBuf[0] != '\0') matched = _nameEquals(lfnBuf, name);

          if (!matched) {
            char shortName[FSABI::FileSystemMaxFileNameLength];

            _formatEntryName(e, shortName);

            matched = _nameEquals(shortName, name);
          }

          if (matched) {
            _fillResolved(e, outEntry, lfnBuf[0] != '\0' ? lfnBuf : nullptr);

            lfnBuf[0] = '\0';

            return true;
          }

          lfnBuf[0] = '\0';
        }
      }

      cluster = _nextCluster(cluster);
    }

    return false;
  }

  bool Volume::_listRootDirectory(
    Servers::FileSystem::ABI::FileSystemDirectory* entries,
    UInt32 maxEntries,
    UInt32& outCount
  ) {
    UInt8 sectorBuf[512];
    UInt32 entriesPerSector = _bytesPerSector / 32;
    char lfnBuf[LFNMaxLength + 1] = {};

    for (UInt32 s = 0; s < _rootDirectorySectors; s++) {
      if (!_readSectors(_rootDirectoryStart + s, 1, sectorBuf)) return false;

      RawDirectoryEntry* raw = reinterpret_cast<RawDirectoryEntry*>(sectorBuf);

      for (UInt32 i = 0; i < entriesPerSector; i++) {
        RawDirectoryEntry& e = raw[i];

        if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryEnd) {
          return true;
        } else if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryDeleted) {
          lfnBuf[0] = '\0';

          continue;
        } else if (e.Attributes == DirectoryAttribute::LongFileName) {
          _accumulateLFN(
            lfnBuf,
            reinterpret_cast<const RawLFNDirectoryEntry&>(e)
          );

          continue;
        }

        _appendVisible(
          e,
          lfnBuf[0] != '\0' ? lfnBuf : nullptr,
          entries,
          maxEntries,
          outCount
        );

        lfnBuf[0] = '\0';

        if (outCount >= maxEntries) return true;
      }
    }

    return true;
  }

  bool Volume::_listSubdirectory(
    UInt16 startCluster,
    Servers::FileSystem::ABI::FileSystemDirectory* entries,
    UInt32 maxEntries,
    UInt32& outCount
  ) {
    UInt8 sectorBuf[512];
    UInt32 entriesPerSector = _bytesPerSector / 32;
    UInt16 cluster = startCluster;
    char lfnBuf[LFNMaxLength + 1] = {};

    while (cluster >= 2 && cluster < ClusterEOF) {
      UInt64 lba = _clusterToLBA(cluster);

      for (UInt32 s = 0; s < _sectorsPerCluster; s++) {
        if (!_readSectors(lba + s, 1, sectorBuf)) return false;

        RawDirectoryEntry* raw = reinterpret_cast<RawDirectoryEntry*>(
          sectorBuf
        );

        for (UInt32 i = 0; i < entriesPerSector; i++) {
          RawDirectoryEntry& e = raw[i];

          if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryEnd) {
            return true;
          } else if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryDeleted) {
            lfnBuf[0] = '\0';

            continue;
          } else if (e.Attributes == DirectoryAttribute::LongFileName) {
            _accumulateLFN(
              lfnBuf,
              reinterpret_cast<const RawLFNDirectoryEntry&>(e)
            );

            continue;
          }

          _appendVisible(
            e,
            lfnBuf[0] != '\0' ? lfnBuf : nullptr,
            entries,
            maxEntries,
            outCount
          );

          lfnBuf[0] = '\0';

          if (outCount >= maxEntries) return true;
        }
      }

      cluster = _nextCluster(cluster);
    }

    return true;
  }

  void Volume::_formatEntryName(const RawDirectoryEntry& raw, char* outName) {
    int n = 0;
    bool nameLower = (raw.NTReserved & 0x08) != 0;
    bool extLower = (raw.NTReserved & 0x10) != 0;

    // copy the 8-character name, stopping at padding spaces
    for (int i = 0; i < 8 && raw.Name[i] != ' '; i++) {
      char c = raw.Name[i];

      if (nameLower && c >= 'A' && c <= 'Z') c += 32;

      outName[n++] = c;
    }

    // append the 3-character extension if present
    if (raw.Extension[0] != ' ') {
      outName[n++] = '.';

      for (int i = 0; i < 3 && raw.Extension[i] != ' '; i++) {
        char c = raw.Extension[i];

        if (extLower && c >= 'A' && c <= 'Z') c += 32;

        outName[n++] = c;
      }
    }

    outName[n] = '\0';
  }

  bool Volume::_nameEquals(const char* a, const char* b) {
    // case-insensitive ASCII comparison
    while (*a && *b) {
      char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
      char cb = (*b >= 'a' && *b <= 'z') ? (*b - 32) : *b;

      if (ca != cb) return false;

      a++;
      b++;
    }

    return *a == '\0' && *b == '\0';
  }

  void Volume::_accumulateLFN(char* lfnBuf, const RawLFNDirectoryEntry& lfn) {
    UInt32 seqNum = lfn.Sequence & 0x3F;

    if (seqNum == 0 || seqNum > 20) return;

    // first physical LFN entry for a new name (bit 6 set), clear the buffer
    // so leftover state from the previous entry doesn't bleed through
    if (lfn.Sequence & 0x40)
      for (UInt32 i = 0; i <= LFNMaxLength; i++)
        lfnBuf[i] = '\0';

    UInt32 pos = (seqNum - 1) * 13;

    // place one UTF-16LE char as its low byte. 0x0000 (null) and 0xFFFF
    // (padding) both signal end-of-name for this entry
    auto put = [&](UInt16 ch) -> bool {
      if (pos >= LFNMaxLength) return false;
      else if (ch == 0x0000 || ch == 0xFFFF) return false;

      lfnBuf[pos++] = static_cast<char>(ch & 0xFF);

      return true;
    };

    if (!put(lfn.Name1[0])) return;
    else if (!put(lfn.Name1[1])) return;
    else if (!put(lfn.Name1[2])) return;
    else if (!put(lfn.Name1[3])) return;
    else if (!put(lfn.Name1[4])) return;
    else if (!put(lfn.Name2[0])) return;
    else if (!put(lfn.Name2[1])) return;
    else if (!put(lfn.Name2[2])) return;
    else if (!put(lfn.Name2[3])) return;
    else if (!put(lfn.Name2[4])) return;
    else if (!put(lfn.Name2[5])) return;
    else if (!put(lfn.Name3[0])) return;
    else if (!put(lfn.Name3[1])) return;
  }

  UInt32 Volume::_directorySize(UInt16 firstCluster) const {
    UInt32 clusterCount = 0;
    UInt16 cluster = firstCluster;

    while (cluster >= 2 && cluster < ClusterEOF) {
      clusterCount++;
      cluster = _nextCluster(cluster);
    }

    return clusterCount * _sectorsPerCluster * _bytesPerSector;
  }

  UInt32 Volume::_contentSize(UInt16 firstCluster) {
    UInt8 sectorBuf[512];
    UInt32 entriesPerSector = _bytesPerSector / 32;
    UInt16 cluster = firstCluster;
    UInt32 total = 0;

    while (cluster >= 2 && cluster < ClusterEOF) {
      UInt64 lba = _clusterToLBA(cluster);

      for (UInt32 s = 0; s < _sectorsPerCluster; s++) {
        if (!_readSectors(lba + s, 1, sectorBuf)) return total;

        auto* raw = reinterpret_cast<RawDirectoryEntry*>(sectorBuf);

        for (UInt32 i = 0; i < entriesPerSector; i++) {
          RawDirectoryEntry& e = raw[i];

          if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryEnd) return total;
          if (static_cast<UInt8>(e.Name[0]) == DirectoryEntryDeleted) continue;
          if (e.Attributes == DirectoryAttribute::LongFileName) continue;
          if (::Quantum::Core::Enum::HasAnyFlag(static_cast<DirectoryAttribute>(e.Attributes), DirectoryAttribute::VolumeLabel))
            continue;
          if (e.Name[0] == '.') continue;

          if (::Quantum::Core::Enum::HasAnyFlag(static_cast<DirectoryAttribute>(e.Attributes), DirectoryAttribute::Directory))
            total += _contentSize(e.FirstCluster);
          else
            total += e.FileSize;
        }
      }

      cluster = _nextCluster(cluster);
    }

    return total;
  }

  void Volume::_fillResolved(
    const RawDirectoryEntry& raw,
    ResolvedEntry* out,
    const char* lfnName
  ) {
    out->Valid = true;
    out->IsDirectory = ::Quantum::Core::Enum::HasAnyFlag(
      static_cast<DirectoryAttribute>(raw.Attributes),
      DirectoryAttribute::Directory
    );
    out->FirstCluster = raw.FirstCluster;
    out->FileSize = out->IsDirectory
      ? _directorySize(raw.FirstCluster)
      : raw.FileSize;

    if (lfnName && lfnName[0] != '\0')
      Core::CString::Copy(lfnName, out->Name, FSABI::FileSystemMaxFileNameLength);
    else
      _formatEntryName(raw, out->Name);
  }

  bool Volume::_appendVisible(
    const RawDirectoryEntry& raw,
    const char* lfnName,
    Servers::FileSystem::ABI::FileSystemDirectory* entries,
    UInt32 maxEntries,
    UInt32& count
  ) {
    // skip LFN entries, volume labels, and . / .. entries
    if (raw.Attributes == DirectoryAttribute::LongFileName)
      return false;
    else if (::Quantum::Core::Enum::HasAnyFlag(static_cast<DirectoryAttribute>(raw.Attributes), DirectoryAttribute::VolumeLabel))
      return false;
    else if (raw.Name[0] == '.')
      return false;
    else if (count >= maxEntries)
      return false;

    Servers::FileSystem::ABI::FileSystemDirectory& out = entries[count];

    if (lfnName && lfnName[0] != '\0')
      Core::CString::Copy(lfnName, out.Name, FSABI::FileSystemMaxFileNameLength);
    else
      _formatEntryName(raw, out.Name);

    if (::Quantum::Core::Enum::HasAnyFlag(static_cast<DirectoryAttribute>(raw.Attributes), DirectoryAttribute::Directory)) {
      out.Type = Servers::FileSystem::ABI::FileSystemEntryType::Directory;
      out.Size = _contentSize(raw.FirstCluster);
    } else {
      out.Type = Servers::FileSystem::ABI::FileSystemEntryType::Regular;
      out.Size = raw.FileSize;
    }

    count++;

    return true;
  }
}
