/**
 * @file System/FileSystems/FAT12/File.cpp
 * @brief FAT12 file system file helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Devices/BlockDevices.hpp>

#include "FAT.hpp"
#include "File.hpp"
#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12 {
  using ABI::Devices::BlockDevices;

  void File::Initialize(Volume& volume) {
    _volume = &volume;
  }

  bool File::Read(
    UInt32 startCluster,
    UInt32 offset,
    UInt8* buffer,
    UInt32 length,
    UInt32& outRead,
    UInt32 fileSize
  ) {
    outRead = 0;

    if (!_volume || !_volume->_valid || !buffer || length == 0) {
      return false;
    }

    if (offset >= fileSize) {
      return true;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

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

    UInt32 clusterSize = bytesPerSector * _volume->_sectorsPerCluster;
    UInt32 skipClusters = offset / clusterSize;
    UInt32 clusterOffset = offset % clusterSize;
    UInt32 cluster = startCluster;

    for (UInt32 i = 0; i < skipClusters; ++i) {
      UInt32 nextCluster = 0;

      if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        return true;
      }

      cluster = nextCluster;
    }

    UInt32 sectorOffset = clusterOffset / bytesPerSector;
    UInt32 byteOffset = clusterOffset % bytesPerSector;

    while (remaining > 0) {
      UInt8 sector[512] = {};
      UInt32 lba
        = _volume->_dataStartLBA
        + (cluster - 2) * _volume->_sectorsPerCluster
        + sectorOffset;
      BlockDevices::Request request {};

      request.deviceId = _volume->_device.id;
      request.lba = lba;
      request.count = 1;
      request.buffer = sector;

      if (BlockDevices::Read(request) != 0) {
        return false;
      }

      UInt32 remainingSector = bytesPerSector - byteOffset;
      UInt32 chunk = remaining < remainingSector ? remaining : remainingSector;

      for (UInt32 i = 0; i < chunk; ++i) {
        buffer[outRead + i] = sector[byteOffset + i];
      }

      outRead += chunk;
      remaining -= chunk;
      byteOffset = 0;
      ++sectorOffset;

      if (remaining == 0) {
        break;
      }

      if (sectorOffset >= _volume->_sectorsPerCluster) {
        UInt32 nextCluster = 0;

        if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
          return false;
        }

        if (FAT::IsEndOfChain(nextCluster)) {
          break;
        }

        cluster = nextCluster;
        sectorOffset = 0;
      }
    }

    return true;
  }

  bool File::Write(
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

    if (!_volume || !_volume->_valid || !buffer || length == 0) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 clusterSize = bytesPerSector * _volume->_sectorsPerCluster;
    UInt32 endOffset = offset + length;

    if (startCluster == 0) {
      UInt32 firstCluster = 0;

      if (!_volume || !_volume->FindFreeCluster(firstCluster)) {
        return false;
      }

      if (!_volume || !_volume->WriteFATEntry(firstCluster, 0xFFF)) {
        return false;
      }

      startCluster = firstCluster;
      if (_volume->_freeClusters > 0) {
        --_volume->_freeClusters;
        _volume->_info.freeSectors
          = _volume->_freeClusters * _volume->_sectorsPerCluster;
      }
    }

    UInt32 clustersNeeded = (endOffset + clusterSize - 1) / clusterSize;
    UInt32 clusterCount = 0;
    UInt32 cluster = startCluster;
    UInt32 lastCluster = startCluster;

    for (;;) {
      ++clusterCount;

      UInt32 nextCluster = 0;

      if (!_volume || !_volume->ReadFATEntry(cluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
        lastCluster = cluster;
        break;
      }

      cluster = nextCluster;
    }

    while (clusterCount < clustersNeeded) {
      UInt32 newCluster = 0;

      if (!_volume || !_volume->FindFreeCluster(newCluster)) {
        return false;
      }

      if (!_volume || !_volume->WriteFATEntry(lastCluster, newCluster)) {
        return false;
      }

      if (!_volume || !_volume->WriteFATEntry(newCluster, 0xFFF)) {
        return false;
      }

      lastCluster = newCluster;
      ++clusterCount;
      if (_volume->_freeClusters > 0) {
        --_volume->_freeClusters;
        _volume->_info.freeSectors
          = _volume->_freeClusters * _volume->_sectorsPerCluster;
      }
    }

    UInt32 skipClusters = offset / clusterSize;
    UInt32 clusterOffset = offset % clusterSize;
    UInt32 currentCluster = startCluster;

    for (UInt32 i = 0; i < skipClusters; ++i) {
      UInt32 nextCluster = 0;

      if (!_volume || !_volume->ReadFATEntry(currentCluster, nextCluster)) {
        return false;
      }

      if (FAT::IsEndOfChain(nextCluster)) {
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
        = _volume->_dataStartLBA
        + (currentCluster - 2) * _volume->_sectorsPerCluster
        + sectorOffset;
      BlockDevices::Request request {};

      request.deviceId = _volume->_device.id;
      request.lba = lba;
      request.count = 1;
      request.buffer = sector;

      if (BlockDevices::Read(request) != 0) {
        return false;
      }

      UInt32 remainingSector = bytesPerSector - byteOffset;
      UInt32 chunk = remaining < remainingSector ? remaining : remainingSector;

      for (UInt32 i = 0; i < chunk; ++i) {
        sector[byteOffset + i] = buffer[outWritten + i];
      }

      request.lba = lba;
      request.buffer = sector;

      if (BlockDevices::Write(request) != 0) {
        return false;
      }

      outWritten += chunk;
      remaining -= chunk;
      byteOffset = 0;
      ++sectorOffset;

      if (remaining == 0) {
        break;
      }

      if (sectorOffset >= _volume->_sectorsPerCluster) {
        UInt32 nextCluster = 0;

        if (!_volume || !_volume->ReadFATEntry(currentCluster, nextCluster)) {
          return false;
        }

        if (FAT::IsEndOfChain(nextCluster)) {
          return false;
        }

        currentCluster = nextCluster;
        sectorOffset = 0;
      }
    }

    if (endOffset > fileSize) {
      outSize = endOffset;
    }

    return true;
  }
}
