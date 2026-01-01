/**
 * @file System/Kernel/Devices/InputDevices.cpp
 * @brief Input device registry and event queue.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Devices/InputDevices.hpp"
#include "Task.hpp"
#include "Sync/ScopedLock.hpp"

namespace Quantum::System::Kernel::Devices {
  using Kernel::Task;
  using Objects::Devices::InputDeviceObject;

  void InputDevices::Initialize() {
    _lock.Initialize();

    _deviceCount = 0;
    _nextDeviceId = 1;

    for (UInt32 i = 0; i < _maxDevices; ++i) {
      _devices[i] = nullptr;
      _deviceStorage[i].info.id = 0;
      _deviceStorage[i].info.flags = 0;
      _deviceStorage[i].info.type = Type::Unknown;
      _deviceStorage[i].info.deviceIndex = 0;
      _deviceStorage[i].ownerId = 0;
      _deviceStorage[i].head = 0;
      _deviceStorage[i].tail = 0;

      _deviceStorage[i].waitQueue.Initialize();

      _deviceStorage[i].object = nullptr;
    }
  }

  UInt32 InputDevices::Register(InputDevices::Device* device) {
    if (!device || _deviceCount >= _maxDevices) {
      return 0;
    }

    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    UInt32 id = _nextDeviceId++;

    device->info.id = id;
    device->info.flags |= flagReady;
    device->ownerId = 0;
    device->head = 0;
    device->tail = 0;

    device->waitQueue.Initialize();

    device->object = new InputDeviceObject(id);

    if (!device->object) {
      device->info.id = 0;

      return 0;
    }

    _devices[_deviceCount++] = device;

    return id;
  }

  UInt32 InputDevices::RegisterUser(const InputDevices::Info& info) {
    if (_deviceCount >= _maxDevices) {
      return 0;
    }

    if (info.type == Type::Unknown) {
      return 0;
    }

    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      Device* device = _devices[i];

      if (!device) {
        continue;
      }

      if (
        device->info.type == info.type
        && device->info.deviceIndex == info.deviceIndex
      ) {
        return 0;
      }
    }

    Device* storage = nullptr;

    for (UInt32 i = 0; i < _maxDevices; ++i) {
      if (_deviceStorage[i].info.id == 0) {
        storage = &_deviceStorage[i];

        break;
      }
    }

    if (!storage) {
      return 0;
    }

    UInt32 id = _nextDeviceId++;

    storage->info = info;
    storage->info.id = id;
    storage->info.flags |= flagReady;
    storage->ownerId = Task::GetCurrentId();
    storage->head = 0;
    storage->tail = 0;

    storage->waitQueue.Initialize();

    storage->object = new InputDeviceObject(id);

    if (!storage->object) {
      storage->info.id = 0;

      return 0;
    }

    _devices[_deviceCount++] = storage;

    return id;
  }

  bool InputDevices::Unregister(UInt32 deviceId) {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_devices[i] && _devices[i]->info.id == deviceId) {
        if (_devices[i]->ownerId != 0) {
          if (_devices[i]->ownerId != Task::GetCurrentId()) {
            return false;
          }
        }

        if (_devices[i]->object) {
          _devices[i]->object->Release();
          _devices[i]->object = nullptr;
        }

        _devices[i]->info.id = 0;
        _devices[i]->info.flags = 0;
        _devices[i]->ownerId = 0;
        _devices[i]->head = 0;
        _devices[i]->tail = 0;
        _devices[i] = _devices[_deviceCount - 1];
        _devices[_deviceCount - 1] = nullptr;
        _deviceCount--;

        return true;
      }
    }

    return false;
  }

  UInt32 InputDevices::GetCount() {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    return _deviceCount;
  }

  bool InputDevices::GetInfo(UInt32 deviceId, InputDevices::Info& outInfo) {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    InputDevices::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    outInfo = device->info;

    return true;
  }

  bool InputDevices::UpdateInfo(
    UInt32 deviceId,
    const InputDevices::Info& info
  ) {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    InputDevices::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    if (info.id != deviceId || info.type != device->info.type) {
      return false;
    }

    if (device->ownerId != 0 && device->ownerId != Task::GetCurrentId()) {
      return false;
    }

    device->info.flags = info.flags;
    device->info.deviceIndex = info.deviceIndex;

    return true;
  }

  bool InputDevices::ReadEvent(UInt32 deviceId, InputDevices::Event& outEvent) {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    InputDevices::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    if ((device->info.flags & flagReady) == 0) {
      return false;
    }

    if (device->head == device->tail) {
      return false;
    }

    outEvent = device->events[device->tail];
    device->tail = (device->tail + 1) % eventQueueSize;

    return true;
  }

  bool InputDevices::PushEvent(
    UInt32 deviceId,
    const InputDevices::Event& event
  ) {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    InputDevices::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    if ((device->info.flags & flagReady) == 0) {
      return false;
    }

    if (device->ownerId != 0 && device->ownerId != Task::GetCurrentId()) {
      return false;
    }

    UInt32 next = (device->head + 1) % eventQueueSize;

    if (next == device->tail) {
      return false;
    }

    InputDevices::Event stored = event;

    stored.deviceId = deviceId;
    device->events[device->head] = stored;
    device->head = next;

    device->waitQueue.WakeOne();

    return true;
  }

  InputDevices::Device* InputDevices::Find(UInt32 deviceId) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_devices[i] && _devices[i]->info.id == deviceId) {
        return _devices[i];
      }
    }

    return nullptr;
  }

  InputDeviceObject* InputDevices::GetObject(UInt32 deviceId) {
    Sync::ScopedLock<Sync::SpinLock> guard(_lock);

    InputDevices::Device* device = Find(deviceId);

    if (!device) {
      return nullptr;
    }

    return device->object;
  }
}
