/**
 * @file Include/Quantum/Core/Resource.hpp
 * @brief Declares @ref @QCore::ResourceID, @ref @QCore::Resource, and
 *        @ref @QCore::ResourceRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IDAllocator.hpp"
#include "Result.hpp"
#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Type for resource identifiers.
   */
  using ResourceID = UInt32;

  /**
   * @brief Minimal base structure for managed resources.
   *
   * Provides the fields that @ref ResourceRepository needs to track a
   * resource slot: a unique @ref ResourceID and an active flag.
   * Domain-specific subtypes add their own payload fields.
   */
  struct Resource {
    /**
     * @brief Unique identifier assigned by a @ref ResourceRepository.
     *
     * Zero is never assigned; a zero ID indicates an uninitialized or
     * freed resource.
     */
    ResourceID ID;

    /**
     * @brief Whether this resource slot is actively in use.
     */
    bool Active;
  };

  /**
   * @brief Fixed-capacity repository for @ref Resource subtypes.
   * @tparam ResourceT The concrete @ref Resource subtype to manage. Must
   *                    inherit from @ref Resource.
   * @tparam Capacity Maximum number of simultaneously live resources.
   *
   * Manages a flat array of `ResourceT` slots with an @ref IDAllocator
   * for \f$\mathcal{O}(1)\f$ ID allocation and free-list reuse.  Lookup
   * is linear over the slot array, matching the kernel's
   * `KernelResourceManager` semantics but without heap allocation.
   */
  template <typename ResourceT, Size Capacity>
  class ResourceRepository {
    public:
      /**
       * @brief Creates a new @ref ResourceRepository.
       *
       * IDs start at `1` so that `0` remains an invalid sentinel.
       */
      ResourceRepository() : _resourceIDs(1) {}

      /**
       * @brief Allocates a new resource slot and assigns it a unique
       *        @ref ResourceID.
       * @return @ref Result containing a pointer to the allocated
       *         `ResourceT` on success, or failure if the repository is
       *         full. The caller must fill in domain-specific fields on the
       *         returned resource before using it.
       */
      Result<ResourceT*> Allocate() {
        ResourceID id;

        if (!_resourceIDs.Allocate(id)) {
          return Result<ResourceT*>(false);
        }

        for (Size index = 0; index < Capacity; index++) {
          if (!_resources[index].Active) {
            _resources[index] = ResourceT{};
            _resources[index].ID = id;
            _resources[index].Active = true;

            return Result<ResourceT*>(true, &_resources[index]);
          }
        }

        _resourceIDs.Free(id);

        return Result<ResourceT*>(false);
      }

      /**
       * @brief Finds a resource by its @ref ResourceID.
       * @param resourceID The @ref ResourceID to look up.
       * @return @ref Result containing a pointer to the matching
       *         `ResourceT`, or failure if no active resource with the
       *         given ID exists.
       */
      Result<ResourceT*> FindByID(ResourceID resourceID) {
        for (Size index = 0; index < Capacity; index++) {
          if (
            _resources[index].Active &&
            _resources[index].ID == resourceID
          ) {
            return Result<ResourceT*>(true, &_resources[index]);
          }
        }

        return Result<ResourceT*>(false);
      }

      /**
       * @brief Finds the first active resource matching a predicate.
       * @tparam Predicate Callable taking `const ResourceT&` and returning
       *                   `bool`.
       * @param predicate Called for each active resource. Return `true` to
       *                  select the resource.
       * @return @ref Result containing a pointer to the first matching
       *         `ResourceT`, or failure if no resource matches.
       */
      template <typename Predicate>
      Result<ResourceT*> FindFirst(Predicate predicate) {
        for (Size index = 0; index < Capacity; index++) {
          if (_resources[index].Active && predicate(_resources[index])) {
            return Result<ResourceT*>(true, &_resources[index]);
          }
        }

        return Result<ResourceT*>(false);
      }

      /**
       * @brief Iterates over all active resources and invokes a callback
       *        for each.
       * @tparam Callback Callable taking `ResourceT&`.
       * @param callback Invoked once for each active resource.
       */
      template <typename Callback>
      void ForEach(Callback callback) {
        for (Size index = 0; index < Capacity; index++) {
          if (_resources[index].Active) {
            callback(_resources[index]);
          }
        }
      }

      /**
       * @brief Removes a resource by its @ref ResourceID.
       * @param resourceID The @ref ResourceID of the resource to remove.
       * @return `true` if the resource was found and removed; `false`
       *         otherwise.
       *
       * Marks the slot as inactive and returns the ID to the free pool.
       */
      bool Remove(ResourceID resourceID) {
        for (Size index = 0; index < Capacity; index++) {
          if (
            _resources[index].Active &&
            _resources[index].ID == resourceID
          ) {
            _resources[index] = ResourceT{};

            _resourceIDs.Free(resourceID);

            return true;
          }
        }

        return false;
      }

    private:
      /**
       * @brief Fixed array of resource slots.
       */
      ResourceT _resources[Capacity] = {};

      /**
       * @brief @ref IDAllocator for @ref ResourceID assignment with
       *        free-list reuse.
       */
      IDAllocator<ResourceID, Capacity> _resourceIDs;
  };
}
