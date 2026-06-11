/**
 * @file Kernel/Memory/ObjectPool.hpp
 * @brief Declares @ref @QKrnl::Memory::ObjectPool.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Structures/IntrusiveFreeNode.hpp>
#include <KernelRuntime.hpp>
#include <KernelTypes.hpp>

#include "HeapAllocator.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Per-type pool allocator providing \f$\mathcal{O}(1)\f$
   *        allocation and free for fixed-size objects.
   * @tparam T The element type.
   * @tparam ElementsPerSlab Number of elements per slab allocation.
   *
   * Each pool manages a linked list of slabs. A slab is a single heap
   * allocation subdivided into `ElementsPerSlab` element slots. Free slots
   * are linked via an intrusive singly-linked list stored in-place within
   * the unused element memory.
   */
  template <typename T, Size ElementsPerSlab = 32>
  class ObjectPool {
    public:
      /**
       * @brief Constructs an empty @ref ObjectPool.
       * @param heap Reference to the @ref HeapAllocator.
       */
      explicit ObjectPool(HeapAllocator& heap) : _heap(heap) {}

      /**
       * @brief Allocates memory for an element.
       * @return `void` pointer to uninitialized element storage, or `nullptr`
       *         if the heap is exhausted.
       * @note The caller must construct the object.
       */
      void* Allocate() {
        // if empty, grow the pool by allocating a new slab and pushing its
        // element slots onto the free list ...
        if (!_freeList) {
          _growPool();
        }

        // ... if it's still empty after trying to grow, the heap is exhausted
        if (!_freeList) {
          return nullptr;
        }

        IntrusiveFreeNode* node = _freeList;

        _freeList = node->Next;
        ++_inUseCount;

        return reinterpret_cast<void*>(node);
      }

      /**
       * @brief Returns an element to the pool.
       * @param element `T` pointer to the element to free.
       * @note The caller must have already called the destructor if `T` is
       *       non-trivial.
       */
      void Free(T* element) {
        if (!element) return;

        IntrusiveFreeNode* node = reinterpret_cast<IntrusiveFreeNode*>(element);

        node->Next = _freeList;
        _freeList = node;
        --_inUseCount;
      }

      /**
       * @brief Gets the number of elements currently in use.
       * @return The in-use element count.
       */
      Size GetInUseCount() const {
        return _inUseCount;
      }

      /**
       * @brief Gets the number of free elements available without allocating a
       *        new slab.
       * @return The free element count.
       */
      Size GetFreeCount() const {
        return _slabCount * ElementsPerSlab - _inUseCount;
      }

      /**
       * @brief Gets the number of slabs allocated from the heap.
       * @return The slab count.
       */
      Size GetSlabCount() const {
        return _slabCount;
      }

      /**
       * @brief Gets the total bytes reserved by all slabs.
       * @return The total byte count across all slabs.
       */
      Size GetTotalBytes() const {
        return _slabCount * SlabSize;
      }

      /**
       * @brief Releases completely free slabs back to the heap.
       * @return The number of bytes freed.
       *
       * Walks all slabs, and for any slab where every element slot is on the
       * free list, removes those slots from the free list and frees the slab.
       */
      Size Trim() {
        Size freedBytes = 0;
        IntrusiveFreeNode** slabPtr = &_slabs;

        while (*slabPtr) {
          IntrusiveFreeNode* slab = *slabPtr;
          UInt8* storage = reinterpret_cast<UInt8*>(slab) + sizeof(IntrusiveFreeNode);
          UInt8* slabEnd = storage + ElementSize * ElementsPerSlab;

          Size freeInSlab = 0;
          IntrusiveFreeNode* node = _freeList;

          while (node) {
            UInt8* address = reinterpret_cast<UInt8*>(node);

            if (
              address >= storage &&
              address < slabEnd
            ) {
              ++freeInSlab;
            }

            node = node->Next;
          }

          if (freeInSlab == ElementsPerSlab) {
            IntrusiveFreeNode** freePtr = &_freeList;

            while (*freePtr) {
              UInt8* address = reinterpret_cast<UInt8*>(*freePtr);

              if (
                address >= storage &&
                address < slabEnd
              ) {
                *freePtr = (*freePtr)->Next;
              } else {
                freePtr = &(*freePtr)->Next;
              }
            }

            *slabPtr = slab->Next;
            --_slabCount;

            _heap.Free(slab);

            freedBytes += SlabSize;
          } else {
            slabPtr = &(*slabPtr)->Next;
          }
        }

        return freedBytes;
      }

    private:
      /**
       * @brief Element size rounded up for alignment and free-list storage.
       */
      static constexpr Size ElementSize =
        (
          (
            sizeof(T) < sizeof(IntrusiveFreeNode)
              ? sizeof(IntrusiveFreeNode)
              : sizeof(T)
          ) + 3
        ) & ~static_cast<Size>(3);

      /**
       * @brief Total allocation size per slab (header + element storage).
       */
      static constexpr Size SlabSize =
        sizeof(IntrusiveFreeNode) + ElementSize * ElementsPerSlab;

      HeapAllocator& _heap;

      IntrusiveFreeNode* _freeList = nullptr;

      IntrusiveFreeNode* _slabs = nullptr;

      Size _inUseCount = 0;

      Size _slabCount = 0;

      /**
       * @brief Allocates a new slab from the heap and pushes all its
       *        element slots onto the free list.
       */
      void _growPool() {
        auto* raw = reinterpret_cast<UInt8*>(_heap.Allocate(SlabSize));

        if (!raw) return;

        auto* slab = reinterpret_cast<IntrusiveFreeNode*>(raw);
        slab->Next = _slabs;
        _slabs = slab;
        ++_slabCount;

        UInt8* storage = raw + sizeof(IntrusiveFreeNode);

        for (Size i = 0; i < ElementsPerSlab; ++i) {
          auto* node = reinterpret_cast<IntrusiveFreeNode*>(
            storage + i * ElementSize
          );
          node->Next = _freeList;
          _freeList = node;
        }
      }
  };
}
