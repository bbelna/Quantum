/**
 * @file System/Kernel/Handles.cpp
 * @brief Kernel handle table implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Handles.hpp"

namespace Quantum::System::Kernel {
  void HandleTable::Initialize() {
    for (UInt32 i = 0; i < maxHandles; ++i) {
      _entries[i].inUse = false;
      _entries[i].type = KernelObject::Type::None;
      _entries[i].rights = 0;
      _entries[i].object = nullptr;
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
    KernelObject::Type type,
    KernelObject* object,
    UInt32 rights
  ) {
    if (!object) {
      return 0;
    }

    if (object->type == KernelObject::Type::None) {
      return 0;
    }

    for (UInt32 i = 0; i < maxHandles; ++i) {
      if (!_entries[i].inUse) {
        Handle handle = handleTag | (i + 1);

        _entries[i].inUse = true;
        _entries[i].type = type;
        _entries[i].rights = rights;
        _entries[i].object = object;
        _entries[i].handle = handle;

        object->AddRef();

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

    if (_entries[index].object) {
      _entries[index].object->Release();
    }

    _entries[index].inUse = false;
    _entries[index].type = KernelObject::Type::None;
    _entries[index].rights = 0;
    _entries[index].object = nullptr;
    _entries[index].handle = 0;

    return true;
  }

  bool HandleTable::Resolve(
    Handle handle,
    KernelObject::Type type,
    UInt32 rights,
    KernelObject*& outObject
  ) const {
    UInt32 index = GetIndex(handle);

    if (index >= maxHandles) {
      return false;
    }

    const Entry& entry = _entries[index];

    if (!entry.inUse || entry.handle != handle) {
      return false;
    }

    if (type != KernelObject::Type::None && entry.type != type) {
      return false;
    }

    if ((entry.rights & rights) != rights) {
      return false;
    }

    outObject = entry.object;

    return true;
  }
}
