/**
 * @file FileSystems/FAT12/HandleManager.cpp
 * @brief Implements the FAT12 server open-file handle manager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "HandleManager.hpp"

namespace Quantum::FileSystems::FAT12 {
  using FileHandle = Servers::FileSystem::ABI::FileHandle;

  FileHandle HandleManager::Open(const ResolvedEntry& entry) {
    for (UInt32 i = 0; i < MaxOpenFiles; i++) {
      if (!_slots[i].Valid) {
        _slots[i].Valid = true;
        _slots[i].Entry = entry;

        return static_cast<FileHandle>(i + 1); // 1-based
      }
    }

    return 0; // table full
  }

  void HandleManager::Close(FileHandle handle) {
    if (handle == 0 || handle > static_cast<FileHandle>(MaxOpenFiles)) return;

    _slots[handle - 1].Valid = false;
  }

  OpenFile* HandleManager::Get(FileHandle handle) {
    if (handle == 0 || handle > static_cast<FileHandle>(MaxOpenFiles))
      return nullptr;

    OpenFile* slot = &_slots[handle - 1];

    return slot->Valid ? slot : nullptr;
  }
}
