/**
 * @file Servers/FileSystem/VolumeTable.cpp
 * @brief Implements @ref @QFSSrv::VolumeTable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "VolumeTable.hpp"

namespace Quantum::Servers::FileSystem {
  FileSystemError VolumeTable::Mount(
    const char* volumeID,
    const char* label,
    IPCPortID servicePortID,
    ProcessID ownerPID,
    UInt32 totalBytes,
    UInt32 usedBytes,
    UInt32 freeBytes
  ) {
    if (!volumeID || !label) return FileSystemError::InvalidPath;

    Size idLength = CString::Length(volumeID);

    if (idLength == 0 || idLength >= FileSystemMaxVolumeIDLength) {
      return FileSystemError::LabelTooLong;
    }

    Size labelLength = CString::Length(label);

    if (labelLength >= FileSystemMaxVolumeLabelLength) {
      return FileSystemError::LabelTooLong;
    }

    // reject duplicate volume IDs
    if (FindByID(volumeID)) return FileSystemError::AlreadyMounted;

    // find an empty slot
    for (Size volumeIndex = 0; volumeIndex < MaxVolumes; volumeIndex++) {
      if (!_volumes[volumeIndex].Active) {
        CString::Copy(
          volumeID,
          _volumes[volumeIndex].VolumeID,
          FileSystemMaxVolumeIDLength
        );
        CString::Copy(
          label,
          _volumes[volumeIndex].Label,
          FileSystemMaxVolumeLabelLength
        );

        _volumes[volumeIndex].FileSystemPortID = servicePortID;
        _volumes[volumeIndex].OwnerPID = ownerPID;
        _volumes[volumeIndex].Active = true;
        _volumes[volumeIndex].TotalBytes = totalBytes;
        _volumes[volumeIndex].UsedBytes = usedBytes;
        _volumes[volumeIndex].FreeBytes = freeBytes;

        _count++;

        return FileSystemError::None;
      }
    }

    return FileSystemError::TableFull;
  }

  FileSystemError VolumeTable::Unmount(const char* volumeID) {
    if (!volumeID) return FileSystemError::NotFound;

    for (Size volumeIndex = 0; volumeIndex < MaxVolumes; volumeIndex++) {
      if (
        _volumes[volumeIndex].Active &&
        CString::Equals(_volumes[volumeIndex].VolumeID, volumeID)
      ) {
        _volumes[volumeIndex].Active = false;

        Byte::Zero(_volumes[volumeIndex].VolumeID, FileSystemMaxVolumeIDLength);
        Byte::Zero(_volumes[volumeIndex].Label, FileSystemMaxVolumeLabelLength);

        _volumes[volumeIndex].FileSystemPortID = 0;
        _volumes[volumeIndex].OwnerPID = 0;

        _count--;

        return FileSystemError::None;
      }
    }

    return FileSystemError::NotFound;
  }

  const Volume* VolumeTable::Resolve(
    const char* prefix,
    FileSystemError* errorOut
  ) const {
    if (!prefix) {
      if (errorOut) *errorOut = FileSystemError::VolumeNotFound;

      return nullptr;
    }

    // try exact volume ID match
    const Volume* idMatch = FindByID(prefix);

    if (idMatch) return idMatch;

    // try unique label match
    const Volume* labelMatch = nullptr;
    Size labelMatchCount = 0;

    for (Size volumeIndex = 0; volumeIndex < MaxVolumes; volumeIndex++) {
      if (
        _volumes[volumeIndex].Active &&
        CString::Equals(_volumes[volumeIndex].Label, prefix)
      ) {
        labelMatch = &_volumes[volumeIndex];
        labelMatchCount++;
      }
    }

    if (labelMatchCount == 1) return labelMatch;

    if (labelMatchCount > 1) {
      if (errorOut) *errorOut = FileSystemError::AmbiguousVolume;

      return nullptr;
    }

    if (errorOut) *errorOut = FileSystemError::VolumeNotFound;

    return nullptr;
  }

  const Volume* VolumeTable::FindByID(const char* volumeID) const {
    if (!volumeID) return nullptr;

    for (Size volumeIndex = 0; volumeIndex < MaxVolumes; volumeIndex++) {
      if (
        _volumes[volumeIndex].Active &&
        CString::Equals(_volumes[volumeIndex].VolumeID, volumeID)
      ) {
        return &_volumes[volumeIndex];
      }
    }

    return nullptr;
  }

  Size VolumeTable::GetActiveVolumes(
    FileSystemVolumeInfo* out,
    Size maxCount
  ) const {
    Size written = 0;

    for (
      Size volumeIndex = 0;
      volumeIndex < MaxVolumes && written < maxCount;
      volumeIndex++
    ) {
      if (_volumes[volumeIndex].Active) {
        CString::Copy(
          _volumes[volumeIndex].VolumeID,
          out[written].VolumeID,
          FileSystemMaxVolumeIDLength
        );
        CString::Copy(
          _volumes[volumeIndex].Label,
          out[written].Label,
          FileSystemMaxVolumeLabelLength
        );

        out[written].TotalBytes = _volumes[volumeIndex].TotalBytes;
        out[written].UsedBytes = _volumes[volumeIndex].UsedBytes;
        out[written].FreeBytes = _volumes[volumeIndex].FreeBytes;

        written++;
      }
    }

    return written;
  }
}
