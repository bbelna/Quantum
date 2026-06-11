/**
 * @file Kernel/Core/KernelModuleRepository.cpp
 * @brief Implements @ref @QKrnl::Core::KernelModuleRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "KernelModuleRepository.hpp"

namespace Quantum::Kernel::Core {
  bool KernelModuleRepository::Register(IKernelModule* module) {
    if (_count >= MaxKernelModules) {
      KLOG_ERROR(
        "Module repository full; cannot register \"%s\"",
        module->GetID().Name
      );

      return false;
    }

    _modules[_count++] = module;

    return true;
  }

  bool KernelModuleRepository::InitializeAll(KernelContext* context) {
    IKernelModule* order[MaxKernelModules] = {};
    Size orderCount = 0;

    if (!_resolveOrder(order, orderCount)) {
      KLOG_ERROR("Module dependency resolution failed (cycle detected)");

      return false;
    }

    for (Size i = 0; i < orderCount; ++i) {
      IKernelModule* module = order[i];

      KLOG_TRACE("Initializing module: %s", module->GetID().Name);

      if (module->Initialize(context)) {
        module->SetState(KernelModuleState::Initialized);
      } else {
        module->SetState(KernelModuleState::Failed);

        KLOG_ERROR(
          "Module \"%s\" failed to initialize",
          module->GetID().Name
        );

        return false;
      }
    }

    return true;
  }

  IKernelModule* KernelModuleRepository::Find(
    const KernelModuleID& id
  ) const {
    Size index = _findIndex(id);

    if (index < _count) {
      return _modules[index];
    }

    return nullptr;
  }

  Size KernelModuleRepository::_findIndex(
    const KernelModuleID& id
  ) const {
    for (Size i = 0; i < _count; ++i) {
      if (_modules[i]->GetID() == id) {
        return i;
      }
    }

    return _count;
  }

  bool KernelModuleRepository::_resolveOrder(
    IKernelModule** order,
    Size& orderCount
  ) {
    UInt8 inDegree[MaxKernelModules] = {};
    Size queue[MaxKernelModules] = {};
    Size queueHead = 0;
    Size queueTail = 0;

    orderCount = 0;

    // compute in-degrees: for each module, count how many of its
    // declared dependencies are also registered in this repository
    for (Size i = 0; i < _count; ++i) {
      KernelModuleID deps[MaxModuleDependencies];
      Size depCount = 0;

      _modules[i]->GetDependencies(deps, depCount);

      for (Size d = 0; d < depCount; ++d) {
        Size depIndex = _findIndex(deps[d]);

        // dependencies on modules not in this repository (e.g.,
        // platform-provided components already in KernelContext)
        // are silently skipped
        if (depIndex < _count) {
          ++inDegree[i];
        }
      }
    }

    // seed the queue with modules that have no in-repository dependencies
    for (Size i = 0; i < _count; ++i) {
      if (inDegree[i] == 0) {
        queue[queueTail++] = i;
      }
    }

    // process the queue (Kahn's algorithm)
    while (queueHead != queueTail) {
      Size idx = queue[queueHead++];

      order[orderCount++] = _modules[idx];

      KernelModuleID resolvedID = _modules[idx]->GetID();

      // for each module that depends on the just-resolved module,
      // decrement its in-degree
      for (Size i = 0; i < _count; ++i) {
        KernelModuleID deps[MaxModuleDependencies];
        Size depCount = 0;

        _modules[i]->GetDependencies(deps, depCount);

        for (Size d = 0; d < depCount; ++d) {
          if (deps[d] == resolvedID) {
            --inDegree[i];

            if (inDegree[i] == 0) {
              queue[queueTail++] = i;
            }
          }
        }
      }
    }

    return orderCount == _count;
  }
}
