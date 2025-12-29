/**
 * @file System/Kernel/KernelObject.cpp
 * @brief Kernel object base definitions.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Heap.hpp"
#include "KernelObject.hpp"

void* operator new(Size /*size*/, void* place) noexcept;

namespace Quantum::System::Kernel {
  void KernelObject::DefaultDestroy(KernelObject* obj) {
    if (obj) {
      Heap::Free(obj);
    }
  }

  KernelObject::KernelObject(
    KernelObject::Type type,
    void (*destroy)(KernelObject*)
  ) {
    this->type = type;
    this->refCount = 1;
    this->destroy = destroy ? destroy : DefaultDestroy;
  }

  void KernelObject::AddRef() {
    ++refCount;
  }

  void KernelObject::Release() {
    if (refCount == 0) {
      return;
    }

    --refCount;

    if (refCount == 0) {
      if (destroy) {
        destroy(this);
      }
    }
  }

  void KernelObject::DestroyIPCPortObject(KernelObject* obj) {
    if (!obj) {
      return;
    }

    auto* portObj = static_cast<IPCPortObject*>(obj);

    portObj->~IPCPortObject();

    Heap::Free(portObj);
  }

  IPCPortObject::IPCPortObject(UInt32 port)
    : KernelObject(
      KernelObject::Type::IPCPort,
      KernelObject::DestroyIPCPortObject
    ),
    portId(port)
  {}

  IPCPortObject* KernelObject::CreateIPCPortObject(UInt32 portId) {
    void* memory = Heap::Allocate(sizeof(IPCPortObject));

    if (!memory) {
      return nullptr;
    }

    return new (memory) IPCPortObject(portId);
  }

  void KernelObject::DestroyBlockDeviceObject(KernelObject* obj) {
    if (!obj) {
      return;
    }

    auto* deviceObj = static_cast<BlockDeviceObject*>(obj);

    deviceObj->~BlockDeviceObject();

    Heap::Free(deviceObj);
  }

  BlockDeviceObject::BlockDeviceObject(UInt32 device)
    : KernelObject(
      KernelObject::Type::BlockDevice,
      KernelObject::DestroyBlockDeviceObject
    ),
    deviceId(device)
  {}

  BlockDeviceObject* KernelObject::CreateBlockDeviceObject(UInt32 deviceId) {
    void* memory = Heap::Allocate(sizeof(BlockDeviceObject));

    if (!memory) {
      return nullptr;
    }

    return new (memory) BlockDeviceObject(deviceId);
  }

  void KernelObject::DestroyInputDeviceObject(KernelObject* obj) {
    if (!obj) {
      return;
    }

    auto* deviceObj = static_cast<InputDeviceObject*>(obj);

    deviceObj->~InputDeviceObject();

    Heap::Free(deviceObj);
  }

  InputDeviceObject::InputDeviceObject(UInt32 device)
    : KernelObject(
      KernelObject::Type::InputDevice,
      KernelObject::DestroyInputDeviceObject
    ),
    deviceId(device)
  {}

  InputDeviceObject* KernelObject::CreateInputDeviceObject(UInt32 deviceId) {
    void* memory = Heap::Allocate(sizeof(InputDeviceObject));

    if (!memory) {
      return nullptr;
    }

    return new (memory) InputDeviceObject(deviceId);
  }

  void KernelObject::DestroyIRQLineObject(KernelObject* obj) {
    if (!obj) {
      return;
    }

    auto* irqObj = static_cast<IRQLineObject*>(obj);

    irqObj->~IRQLineObject();

    Heap::Free(irqObj);
  }

  IRQLineObject::IRQLineObject(UInt32 irq)
    : KernelObject(
      KernelObject::Type::IRQLine,
      KernelObject::DestroyIRQLineObject
    ),
    irqLine(irq)
  {}

  IRQLineObject* KernelObject::CreateIRQLineObject(UInt32 irq) {
    void* memory = Heap::Allocate(sizeof(IRQLineObject));

    if (!memory) {
      return nullptr;
    }

    return new (memory) IRQLineObject(irq);
  }
}
