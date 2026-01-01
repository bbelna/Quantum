/**
 * @file System/Kernel/Objects/KernelObject.cpp
 * @brief Kernel object base.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Objects/KernelObject.hpp"

namespace Quantum::System::Kernel::Objects {
  KernelObject::KernelObject(KernelObjectType type) {
    this->type = type;
    _refCount.Initialize();
  }

  KernelObject::~KernelObject() = default;

  void KernelObject::AddRef() {
    _refCount.AddRef();
  }

  void KernelObject::Release() {
    if (_refCount.Get() == 0) {
      return;
    }

    if (_refCount.Release() == 0) {
      delete this;
    }
  }
}
