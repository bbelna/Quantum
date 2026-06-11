/**
 * @file Kernel/Resources/KernelResourceManager.cpp
 * @brief Implements @ref @QKrnl::Resources::KernelResourceManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "KernelResourceManager.hpp"

namespace Quantum::Kernel::Resources {
  Result<KernelResourceBase*> KernelResourceManager::Remove(
    ResourceID resourceID
  ) {
    Result<LinkedNode<KernelResourceBase*>*> findResult = _getNodeByID(
      resourceID
    );

    if (findResult.Success) {
      KernelResourceBase* resource = findResult.Data->GetValue();

      _resources.Remove(findResult.Data);

      delete findResult.Data;

      _resourceIDs.Free(resourceID);

      return Result<KernelResourceBase*>(
        true,
        resource
      );
    } else {
      return Result<KernelResourceBase*>(false);
    }
  }

  Result<KernelResourceBase*> KernelResourceManager::GetByID(
    ResourceID resourceID
  ) {
    LinkedNode<KernelResourceBase*>* current = _resources.GetHead();

    while (current) {
      KernelResourceBase* resource = current->GetValue();

      if (
        resource &&
        resource->ID == resourceID
      ) {
        return Result<KernelResourceBase*>(
          true,
          resource
        );
      }

      current = current->GetNext();
    }

    return Result<KernelResourceBase*>(false);
  }

  Result<LinkedNode<KernelResourceBase*>*> KernelResourceManager::_getNodeByID(
    ResourceID resourceID
  ) {
    LinkedNode<KernelResourceBase*>* current = _resources.GetHead();

    while (current) {
      KernelResourceBase* resource = current->GetValue();

      if (
        resource &&
        resource->ID == resourceID
      ) {
        return Result<LinkedNode<KernelResourceBase*>*>(
          true,
          current
        );
      }

      current = current->GetNext();
    }

    return Result<LinkedNode<KernelResourceBase*>*>(false);
  }
}
