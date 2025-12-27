/**
 * @file System/FileSystems/FAT12/FAT.cpp
 * @brief FAT12 file system FAT table helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/Devices/BlockDevices.hpp>

#include "FAT.hpp"
#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12 {
  using ABI::Devices::BlockDevices;

  void FAT::Initialize(Volume& volume) {
    _volume = &volume;
  }

  bool FAT::LoadCache() {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 fatBytes = _volume->_fatSectors * bytesPerSector;

    if (fatBytes == 0 || fatBytes > sizeof(_volume->_fatCache)) {
      return false;
    }

    BlockDevices::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = _volume->_fatStartLBA;
    request.count = _volume->_fatSectors;
    request.buffer = _volume->_fatCache;

    if (BlockDevices::Read(request) != 0) {
      return false;
    }

    _volume->_fatCacheBytes = fatBytes;
    _volume->_fatCached = true;

    return true;
  }

  bool FAT::ReadEntry(UInt32 cluster, UInt32& nextCluster) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    if (ReadEntryCached(cluster, nextCluster)) {
      return true;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 fatOffset = cluster + (cluster / 2);
    UInt32 sectorOffset = fatOffset / bytesPerSector;
    UInt32 byteOffset = fatOffset % bytesPerSector;

    if (sectorOffset >= _volume->_fatSectors) {
      return false;
    }

    UInt8 sector[512] = {};
    BlockDevices::Request request {};

    request.deviceId = _volume->_device.id;
    request.lba = _volume->_fatStartLBA + sectorOffset;
    request.count = 1;
    request.buffer = sector;

    if (BlockDevices::Read(request) != 0) {
      return false;
    }

    UInt16 value = 0;

    if (byteOffset == bytesPerSector - 1) {
      UInt8 nextSector[512] = {};

      request.lba = _volume->_fatStartLBA + sectorOffset + 1;
      request.buffer = nextSector;

      if (BlockDevices::Read(request) != 0) {
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

  bool FAT::ReadEntryCached(UInt32 cluster, UInt32& nextCluster) const {
    if (!_volume || !_volume->_fatCached || _volume->_fatCacheBytes == 0) {
      return false;
    }

    UInt32 fatOffset = cluster + (cluster / 2);

    if (fatOffset + 1 >= _volume->_fatCacheBytes) {
      return false;
    }

    UInt16 value = static_cast<UInt16>(
      _volume->_fatCache[fatOffset]
      | (static_cast<UInt16>(_volume->_fatCache[fatOffset + 1]) << 8)
    );

    if ((cluster & 1u) != 0) {
      nextCluster = static_cast<UInt32>(value >> 4);
    } else {
      nextCluster = static_cast<UInt32>(value & 0x0FFF);
    }

    return true;
  }

  bool FAT::WriteEntry(UInt32 cluster, UInt32 value) {
    if (!_volume || !_volume->_valid) {
      return false;
    }

    UInt32 bytesPerSector = _volume->_info.sectorSize;

    if (bytesPerSector != 512) {
      // only support 512-byte sectors for now
      return false;
    }

    UInt32 fatOffset = cluster + (cluster / 2);
    UInt32 sectorOffset = fatOffset / bytesPerSector;
    UInt32 byteOffset = fatOffset % bytesPerSector;

    if (sectorOffset >= _volume->_fatSectors) {
      return false;
    }

    UInt16 packed = static_cast<UInt16>(value & 0x0FFF);

    if (_volume->_fatCached && fatOffset + 1 < _volume->_fatCacheBytes) {
      UInt16 existing = static_cast<UInt16>(
        _volume->_fatCache[fatOffset]
        | (static_cast<UInt16>(_volume->_fatCache[fatOffset + 1]) << 8)
      );

      if ((cluster & 1u) != 0) {
        existing = static_cast<UInt16>((existing & 0x000F) | (packed << 4));
      } else {
        existing = static_cast<UInt16>((existing & 0xF000) | packed);
      }

      _volume->_fatCache[fatOffset] = static_cast<UInt8>(existing & 0xFF);
      _volume->_fatCache[fatOffset + 1]
        = static_cast<UInt8>((existing >> 8) & 0xFF);
    }

    for (UInt32 fatIndex = 0; fatIndex < _volume->_fatCount; ++fatIndex) {
      UInt32 fatLBA
        = _volume->_fatStartLBA
        + fatIndex * _volume->_fatSectors
        + sectorOffset;
      UInt8 sector[512] = {};
      BlockDevices::Request request {};

      request.deviceId = _volume->_device.id;
      request.lba = fatLBA;
      request.count = 1;
      request.buffer = sector;

      if (BlockDevices::Read(request) != 0) {
        return false;
      }

      if (byteOffset == bytesPerSector - 1) {
        UInt8 nextSector[512] = {};

        request.lba = fatLBA + 1;
        request.buffer = nextSector;

        if (BlockDevices::Read(request) != 0) {
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

        if (BlockDevices::Write(request) != 0) {
          return false;
        }

        request.lba = fatLBA + 1;
        request.buffer = nextSector;

        if (BlockDevices::Write(request) != 0) {
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

        if (BlockDevices::Write(request) != 0) {
          return false;
        }
      }
    }

    return true;
  }

  bool FAT::FindFreeCluster(UInt32& outCluster) {
    if (!_volume || !_volume->_valid || _volume->_clusterCount == 0) {
      return false;
    }

    UInt32 maxCluster = _volume->_clusterCount + 1;
    UInt32 start = _volume->_nextFreeCluster;

    if (start < 2 || start > maxCluster) {
      start = 2;
    }

    for (UInt32 cluster = start; cluster <= maxCluster; ++cluster) {
      UInt32 nextCluster = 0;

      if (_volume->_fatCached) {
        if (!ReadEntryCached(cluster, nextCluster)) {
          return false;
        }
      } else {
        if (!ReadEntry(cluster, nextCluster)) {
          return false;
        }
      }

      if (nextCluster == 0x000) {
        outCluster = cluster;
        _volume->_nextFreeCluster = cluster + 1;

        return true;
      }
    }

    for (UInt32 cluster = 2; cluster < start; ++cluster) {
      UInt32 nextCluster = 0;

      if (_volume->_fatCached) {
        if (!ReadEntryCached(cluster, nextCluster)) {
          return false;
        }
      } else {
        if (!ReadEntry(cluster, nextCluster)) {
          return false;
        }
      }

      if (nextCluster == 0x000) {
        outCluster = cluster;
        _volume->_nextFreeCluster = cluster + 1;

        return true;
      }
    }

    return false;
  }

  bool FAT::CountFreeClusters(UInt32& outCount) {
    outCount = 0;

    if (!_volume || !_volume->_valid) {
      return false;
    }

    if (_volume->_clusterCount == 0) {
      return true;
    }

    UInt32 maxCluster = _volume->_clusterCount + 1;

    for (UInt32 cluster = 2; cluster <= maxCluster; ++cluster) {
      UInt32 nextCluster = 0;

      if (_volume->_fatCached) {
        if (!ReadEntryCached(cluster, nextCluster)) {
          return false;
        }
      } else {
        if (!ReadEntry(cluster, nextCluster)) {
          return false;
        }
      }

      // free clusters are marked with 0x000
      if (nextCluster == 0x000) {
        ++outCount;
      }
    }

    return true;
  }

  bool FAT::FreeClusterChain(UInt32 startCluster) {
    if (startCluster < 2) {
      return true;
    }

    UInt32 cluster = startCluster;

    for (;;) {
      UInt32 nextCluster = 0;

      if (!ReadEntry(cluster, nextCluster)) {
        return false;
      }

      if (!WriteEntry(cluster, 0x000)) {
        return false;
      }

      ++_volume->_freeClusters;
      _volume->_info.freeSectors
        = _volume->_freeClusters * _volume->_sectorsPerCluster;

      if (IsEndOfChain(nextCluster)) {
        break;
      }

      cluster = nextCluster;
    }

    return true;
  }

  bool FAT::IsEndOfChain(UInt32 value) {
    return value >= 0xFF8;
  }
}
