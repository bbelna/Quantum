/**
 * @file Kernel/Memory/AddressSpaceMap.hpp
 * @brief Declares @ref @QKrnl::Memory::AddressSpaceMap.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelConstants.hpp>
#include <KernelTypes.hpp>

#include "MemoryMapping.hpp"
#include "MemoryRegion.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Generic ordered map of @ref IAddressSpace occupancy entries.
   * @tparam T
   *   Element type stored in the map. Must provide @c GetBlock() returning a
   *   @c const @ref MemoryBlock&.
   * @tparam MinAddress
   *   Lower bound for @ref FindAvailableBlock; addresses below this are
   *   skipped. Used by per-process maps to reserve the null page.
   *
   * Used by both per-process address-space tracking and the kernel's
   * identity-mapped address space. The element type @c T must expose its
   * @ref MemoryBlock via a @c GetBlock() member returning a `const`
   * reference.
   */
  template <typename T, UIntPtr MinAddress = 0>
  class AddressSpaceMap {
    public:
      /**
       * @brief Inserts an entry into the map.
       * @param entry The entry to insert.
       * @return `true` if the entry was successfully inserted; `false` if a
       *         conflicting entry already exists.
       */
      bool Insert(T entry) {
        UInt32 blockEnd
          = entry.GetBlock().Base
          + entry.GetBlock().SizeInBytes;
        RedBlackNode<T>* current = _map.GetRoot();
        RedBlackNode<T>* predecessor = nullptr;
        RedBlackNode<T>* successor = nullptr;

        while (current) {
          T existing = current->GetValue();

          if (entry.GetBlock().Base < existing.GetBlock().Base) {
            successor = current;
            current = current->GetLeft();
          } else if (existing.GetBlock().Base < entry.GetBlock().Base) {
            predecessor = current;
            current = current->GetRight();
          } else {
            return false;
          }
        }

        if (predecessor) {
          T predecessorValue = predecessor->GetValue();
          UInt32 predecessorEnd
            = predecessorValue.GetBlock().Base
            + predecessorValue.GetBlock().SizeInBytes;

          if (predecessorEnd > entry.GetBlock().Base) {
            return false;
          }
        }

        if (successor) {
          T successorValue = successor->GetValue();

          if (blockEnd > successorValue.GetBlock().Base) {
            return false;
          }
        }

        _map.Insert(entry);

        return true;
      }

      /**
       * @brief Removes an entry from the map.
       * @param entry The entry to remove.
       * @return `true` if the entry was found and removed; `false`
       *         otherwise.
       */
      bool Remove(T entry) {
        RedBlackNode<T>* node = _map.Search(entry);

        if (node) {
          T found = node->GetValue();

          return found.GetBlock().SizeInBytes != entry.GetBlock().SizeInBytes
            ? false
            : _map.Remove(entry);
        } else {
          return false;
        }
      }

      /**
       * @brief Finds the entry that contains a given address.
       * @param address The address to search for.
       * @return A @ref Result containing the found entry if successful, or an
       *         error if no entry contains the address.
       */
      Result<T> FindContainingBlock(UIntPtr address) const {
        RedBlackNode<T>* current = _map.GetRoot();

        while (current) {
          T entry = current->GetValue();
          UInt32 blockEnd
            = entry.GetBlock().Base
            + entry.GetBlock().SizeInBytes;

          if (address < entry.GetBlock().Base) {
            current = current->GetLeft();
          } else if (address >= blockEnd) {
            current = current->GetRight();
          } else {
            _lastFound = entry;

            return Result<T>(true, _lastFound);
          }
        }

        return Result<T>(false);
      }

      /**
       * @brief Finds an available address block of a given size and alignment.
       * @param sizeInBytes The size in bytes of the block to find.
       * @param alignment The required alignment of the block (power of two).
       * @return A @ref Result containing the found block if successful, or an
       *         error if no suitable block is available.
       */
      Result<MemoryBlock> FindAvailableBlock(
        Size sizeInBytes,
        Size alignment
      ) const {
        RedBlackNode<T>* stack[MaxTreeDepth];
        RedBlackNode<T>* current = _map.GetRoot();
        UIntPtr previousEnd = MinAddress;
        Size top = 0;

        while (
          current ||
          top > 0
        ) {
          while (current) {
            stack[top++] = current;
            current = current->GetLeft();
          }

          current = stack[--top];

          T entry = current->GetValue();
          UIntPtr alignedStart = AlignUp(
            previousEnd,
            alignment
          );

          if (
            alignedStart >= previousEnd &&
            alignedStart + sizeInBytes <= entry.GetBlock().Base &&
            alignedStart + sizeInBytes >= alignedStart
          ) {
            return Result<MemoryBlock>(
              true,
              MemoryBlock {
                alignedStart,
                sizeInBytes
              }
            );
          }

          UIntPtr blockEnd
            = entry.GetBlock().Base
            + entry.GetBlock().SizeInBytes;

          if (blockEnd > previousEnd) {
            previousEnd = blockEnd;
          }

          current = current->GetRight();
        }

        UIntPtr alignedStart = AlignUp(
          previousEnd,
          alignment
        );

        if (
          alignedStart >= previousEnd &&
          alignedStart + sizeInBytes >= alignedStart
        ) {
          return Result<MemoryBlock>(
            true,
            MemoryBlock {
              alignedStart,
              sizeInBytes
            }
          );
        }

        return Result<MemoryBlock>(false);
      }

      /**
       * @brief Iterates all entries in the map in order and invokes the given
       *        callback for each one.
       * @tparam Callback Callable taking `const T&`.
       * @param callback The function to invoke for each entry.
       */
      template <typename Callback>
      void ForEach(Callback callback) {
        RedBlackNode<T>* stack[MaxTreeDepth];
        RedBlackNode<T>* current = _map.GetRoot();
        Size top = 0;

        while (
          current ||
          top > 0
        ) {
          while (current) {
            stack[top++] = current;
            current = current->GetLeft();
          }

          current = stack[--top];
          callback(current->GetValue());
          current = current->GetRight();
        }
      }

      /**
       * @brief Gets the underlying red-black tree.
       * @return The red-black tree of entries.
       */
      inline RedBlackTree<T> GetMap() {
        return _map;
      }

    private:
      /**
       * @brief @ref RedBlackTree of entries keyed by base address.
       */
      RedBlackTree<T> _map;

      /**
       * @brief Cached result for pointer-returning queries.
       */
      mutable T _lastFound;
  };

  /**
   * @brief Per-process address-space map.
   * @note The null page (the first 4 KB) is reserved.
   *
   * Tracks process virtual address blocks, paired with the kernel virtual
   * address blocks they alias and the metadata describing the mapping.
   */
  using ProcessAddressSpaceMap = AddressSpaceMap<
    MemoryMapping,
    MemoryBlockSize
  >;

  /**
   * @brief Kernel address-space map.
   *
   * Tracks occupied regions of the kernel's identity-mapped address space.
   */
  using KernelAddressSpaceMap = AddressSpaceMap<MemoryRegion, 0>;
}
