/**
 * @file Kernel/Resources/KernelResourceManager.hpp
 * @brief Declares @ref @QKrnl::Resources::ResourceManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "KernelResource.hpp"

namespace Quantum::Kernel::Resources {
  /**
   * @brief Max number of @ref KernelResourceBase instances that
   *        @ref KernelResourceManager supports.
   */
  constexpr Size MaxResources = 4096;

  /**
   * @brief Manages @ref KernelResourceBase instances.
   *
   * Maintains a flat list of @ref KernelResourceBase instances across all
   * possible values for @ref KernelResourceType with a single monotonically
   * increasing ID namespace (@ref IDAllocator).
   *
   * This manager handles resource ID allocation, storage, lookup, and removal.
   * All domain-specific logic (access control, exclusivity checks, cleanup) is
   * the responsibility of the caller.
   */
  class KernelResourceManager {
    public:
      /**
       * @brief Creates a new @ref KernelResourceManager instance.
       */
      KernelResourceManager() : _resourceIDs(1) {}

      /**
       * @brief Creates a new kernel resource instance (a subtype of
       *        @ref KernelResourceBase, represented by `ResourceT`) with the
       *        given parameters.
       * @tparam ResourceT The concrete @ref KernelResourceBase subtype to
       *                   create.
       * @param type The @ref KernelResourceType for this resource.
       * @param objectID The domain-specific object ID.
       * @param rights The rights associated with this kernel resource.
       * @param ownerPID The @ref ProcessID of the owning @ref Process.
       * @return @ref Result containing the new kernel resource instance
       *         (a `ResourceT*`), or failure.
       */
      template <typename ResourceT>
      Result<ResourceT*> Create(
        KernelResourceType type,
        decltype(ResourceT::ObjectID) objectID,
        decltype(ResourceT::Rights) rights,
        ProcessID ownerPID
      ) {
        auto* resource = new ResourceT();

        ResourceID id;

        if (!_resourceIDs.Allocate(id)) {
          delete resource;

          return Result<ResourceT*>(false);
        }

        resource->ID = id;
        resource->Type = type;
        resource->OwnerPID = ownerPID;
        resource->ObjectID = objectID;
        resource->Rights = rights;

        resource->IncrementRefCount();

        _resources.Append(
          new LinkedNode<KernelResourceBase*>(
            static_cast<KernelResourceBase*>(resource)
          )
        );

        return Result<ResourceT*>(true, resource);
      }

      /**
       * @brief Removes a kernel resource instance (a subtype of
       *        @ref KernelResourceBase) from this @ref KernelResourceManager by
       *        its @ref ResourceID.
       * @param resourceID @ref ResourceID of the kernel resource to remove.
       * @return @ref Result containing the removed kernel resource instance (a
       *         @ref KernelResourceBase pointer), or failure if no kernel
       *         resource with the given @ref ResourceID exists.
       *
       * The caller is responsible for any domain-specific cleanup and for
       * managing the resource's remaining lifetime (e.g., decrementing the
       * reference count).
       */
      Result<KernelResourceBase*> Remove(ResourceID resourceID);

      /**
       * @brief Retrieves a kernel resource (a @ref KernelResourceBase pointer)
       *        by its @ref ResourceID.
       * @param resourceID The @ref ResourceID to look up.
       * @return @ref Result containing the kernel resource (a
       *         @ref KernelResourceBase pointer), or failure.
       */
      Result<KernelResourceBase*> GetByID(ResourceID resourceID);

      /**
       * @brief Type-safe retrieval by @ref ResourceID: returns a failure
       *        @ref Result if the kernel resource instance (a subtype of
       *        @ref KernelResourceBase, represented by `ResourceT`) exists but
       *        has a different @ref KernelResourceType.
       * @tparam ResourceT The expected concrete @ref KernelResourceBase
       *                   subtype.
       * @param resourceID The @ref ResourceID to look up.
       * @param expectedType The expected @ref KernelResourceType tag.
       * @return @ref Result containing a kernel resource instance (a
       *         `ResourceT*`), or failure.
       */
      template <typename ResourceT>
      Result<ResourceT*> GetTyped(
        ResourceID resourceID,
        KernelResourceType expectedType
      ) {
        Result<KernelResourceBase*> result = GetByID(resourceID);

        if (!result.Success) return Result<ResourceT*>(false);

        if (result.Data->Type != expectedType) {
          return Result<ResourceT*>(false);
        }

        return Result<ResourceT*>(
          true,
          static_cast<ResourceT*>(result.Data)
        );
      }

      /**
       * @brief Finds the first kernel resource instance (a subtype of
       *        @ref KernelResourceBase, represented by `ResourceT`) matching
       *        the given @ref KernelResourceType and object ID.
       * @tparam ResourceT The concrete @ref KernelResourceBase subtype.
       * @param type The @ref KernelResourceType to filter by.
       * @param objectID The object ID (`ResourceT::ObjectID`) to match.
       * @return @ref Result containing a kernel resource instance
       *         (a `ResourceT*`), or failure.
       */
      template <typename ResourceT>
      Result<ResourceT*> FindByObjectID(
        KernelResourceType type,
        decltype(ResourceT::ObjectID) objectID
      ) {
        LinkedNode<KernelResourceBase*>* current = _resources.GetHead();

        while (current) {
          KernelResourceBase* base = current->GetValue();

          if (base && base->Type == type) {
            ResourceT* resource = static_cast<ResourceT*>(base);

            if (resource->ObjectID == objectID) {
              return Result<ResourceT*>(true, resource);
            }
          }

          current = current->GetNext();
        }

        return Result<ResourceT*>(false);
      }

      /**
       * @brief Iterates over all kernel resources instances (subtypes of
       *        @ref KernelResourceBase), and invokes @p predicate with 
       *        instances that match the given @ref KernelResourceType
       *        (specified by `ResourceT`) and @p objectID.
       * @tparam ResourceT The concrete @ref KernelResourceBase subtype.
       * @tparam Predicate A callable taking `ResourceT*` and returning
       *                   `bool`. Return `true` to continue iteration, `false`
       *                   to stop early.
       * @param type The @ref KernelResourceType to filter by.
       * @param objectID The object ID (`ResourceT::ObjectID`) to match.
       * @param predicate Called for each matching kernel resource instance.
       * @return `true` if iteration completed without early stop; `false` if
       *         the predicate stopped it early.
       *
       * Useful for exclusivity checks.
       */
      template <typename ResourceT, typename Predicate>
      bool ForEachMatching(
        KernelResourceType type,
        decltype(ResourceT::ObjectID) objectID,
        Predicate predicate
      ) {
        LinkedNode<KernelResourceBase*>* current = _resources.GetHead();

        while (current) {
          KernelResourceBase* base = current->GetValue();

          if (base && base->Type == type) {
            ResourceT* resource = static_cast<ResourceT*>(base);

            if (resource->ObjectID == objectID) {
              if (!predicate(resource)) return false;
            }
          }

          current = current->GetNext();
        }

        return true;
      }

      /**
       * @brief Removes all kernel resource instances (subtypes of
       *        @ref KernelResourceBase) owned by the given @ref ProcessID and
       *        invokes a cleanup @p callback for each.
       * @tparam Callback Callable taking a @ref KernelResourceBase pointer.
       * @param ownerPID The @ref ProcessID whose kernel resource instances
       *                 should be removed.
       * @param callback Invoked with each removed kernel resource instance.
       *
       * @p callback is responsible for any domain-specific cleanup (e.g.,
       * decrementing reference counts, unregistering ports).
       */
      template <typename Callback>
      void RemoveByOwner(ProcessID ownerPID, Callback callback) {
        LinkedNode<KernelResourceBase*>* current = _resources.GetHead();

        while (current) {
          LinkedNode<KernelResourceBase*>* next = current->GetNext();
          KernelResourceBase* base = current->GetValue();

          if (base && base->OwnerPID == ownerPID) {
            _resources.Remove(current);

            delete current;

            _resourceIDs.Free(base->ID);

            callback(base);
          }

          current = next;
        }
      }

    private:
      /**
       * @brief List of all active resources (represented by
       *        @ref KernelResourceBase instances).
       */
      LinkedList<KernelResourceBase*> _resources;

      /**
       * @brief @ref IDAllocator for @ref ResourceID with free list reuse.
       */
      IDAllocator<ResourceID, MaxResources> _resourceIDs;

      /**
       * @brief Finds a @ref KernelResourceBase node in the list by its
       *        @ref KernelResourceBase ID.
       * @param resourceID ID of the resource to find.
       * @return Result containing a pointer to the
       *         @ref LinkedNode<KernelResourceBase*> if found, or failure if no
       *         @ref KernelResourceBase with the specified ID exists.
       */
      Result<LinkedNode<KernelResourceBase*>*> _getNodeByID(
        ResourceID resourceID
      );
  };
}
