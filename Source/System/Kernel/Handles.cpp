/**
 * @file System/Kernel/Handles.cpp
 * @brief Kernel handle table implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Handles.hpp"

namespace Quantum::System::Kernel {
  void HandleTable::Initialize() {
    for (UInt32 i = 0; i < maxHandles; ++i) {
      _entries[i].inUse = false;
      _entries[i].type = ObjectType::None;
      _entries[i].rights = 0;
      _entries[i].objectId = 0;
      _entries[i].handle = 0;
    }

  }

  bool HandleTable::IsHandle(Handle value) {
    return (value & handleTag) != 0;
  }

  UInt32 HandleTable::GetIndex(Handle handle) {
    if (!IsHandle(handle)) {
      return maxHandles;
    }

    UInt32 index = (handle & ~handleTag);

    if (index == 0) {
      return maxHandles;
    }

    return index - 1;
  }

  HandleTable::Handle HandleTable::Create(
    ObjectType type,
    UInt32 objectId,
    UInt32 rights
  ) {
    for (UInt32 i = 0; i < maxHandles; ++i) {
      if (!_entries[i].inUse) {
        Handle handle = handleTag | (i + 1);

        _entries[i].inUse = true;
        _entries[i].type = type;
        _entries[i].rights = rights;
        _entries[i].objectId = objectId;
        _entries[i].handle = handle;

        return handle;
      }
    }

    return 0;
  }

  bool HandleTable::Close(Handle handle) {
    UInt32 index = GetIndex(handle);

    if (index >= maxHandles) {
      return false;
    }

    if (!_entries[index].inUse || _entries[index].handle != handle) {
      return false;
    }

    _entries[index].inUse = false;
    _entries[index].type = ObjectType::None;
    _entries[index].rights = 0;
    _entries[index].objectId = 0;
    _entries[index].handle = 0;

    return true;
  }

  bool HandleTable::Resolve(
    Handle handle,
    ObjectType type,
    UInt32 rights,
    UInt32& outObjectId
  ) const {
    UInt32 index = GetIndex(handle);

    if (index >= maxHandles) {
      return false;
    }

    const Entry& entry = _entries[index];

    if (!entry.inUse || entry.handle != handle) {
      return false;
    }

    if (type != ObjectType::None && entry.type != type) {
      return false;
    }

    if ((entry.rights & rights) != rights) {
      return false;
    }

    outObjectId = entry.objectId;

    return true;
  }
}
