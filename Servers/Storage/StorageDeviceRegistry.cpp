/**
 * @file Servers/Storage/StorageDeviceRegistry.cpp
 * @brief Implements the storage device registry.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StorageDeviceRegistry.hpp"

namespace Quantum::Servers::Storage {
  bool StorageDeviceRegistry::Register(StorageDevice* device) {
    if (!device || _count >= ABI::MaxDevices) return false;

    for (UInt32 i = 0; i < ABI::MaxDevices; i++) {
      if (_devices[i] != nullptr) continue;

      device->_id                  = _nextID;
      device->_descriptor.DeviceID = _nextID;
      _nextID++;
      _devices[i]                  = device;
      _count++;

      return true;
    }

    return false;
  }

  void StorageDeviceRegistry::Unregister(UInt32 id) {
    for (UInt32 i = 0; i < ABI::MaxDevices; i++) {
      if (!_devices[i] || _devices[i]->GetID() != id) continue;

      _devices[i] = nullptr;
      _count--;

      return;
    }
  }

  StorageDevice* StorageDeviceRegistry::Find(UInt32 id) const {
    for (UInt32 i = 0; i < ABI::MaxDevices; i++) {
      if (_devices[i] && _devices[i]->GetID() == id) return _devices[i];
    }

    return nullptr;
  }

  StorageDevice* StorageDeviceRegistry::GetAt(UInt32 index) const {
    UInt32 seen = 0;

    for (UInt32 i = 0; i < ABI::MaxDevices; i++) {
      if (!_devices[i]) continue;

      if (seen == index) return _devices[i];

      seen++;
    }

    return nullptr;
  }
}
