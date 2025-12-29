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
  static void DefaultDestroy(KernelObject* obj) {
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

  static void DestroyIPCPortObject(KernelObject* obj) {
    if (!obj) {
      return;
    }

    auto* portObj = static_cast<IPCPortObject*>(obj);

    portObj->~IPCPortObject();
    Heap::Free(portObj);
  }

  IPCPortObject::IPCPortObject(UInt32 port)
  : KernelObject(KernelObject::Type::IPCPort, DestroyIPCPortObject),
    portId(port) {
  }

  IPCPortObject* KernelObject::CreateIPCPortObject(UInt32 portId) {
    void* memory = Heap::Allocate(sizeof(IPCPortObject));

    if (!memory) {
      return nullptr;
    }

    return new (memory) IPCPortObject(portId);
  }
}
