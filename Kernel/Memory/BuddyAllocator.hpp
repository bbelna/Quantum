/**
 * @file Kernel/Memory/BuddyAllocator.hpp
 * @brief Declares @ref @QKrnl::Memory::BuddyAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "BuddyConstants.hpp"
#include "BuddyFreeNode.hpp"
#include "FreeMemoryBlockListHead.hpp"
#include "Handlers/IMaydayHandler.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Architecture-independent buddy allocator.
   * @tparam Address The address type (e.g. `UInt32` for IA-32).
   *
   * Manages a pool of fixed-size blocks using the buddy system. Supports
   * both top-down (high addresses first) and bottom-up allocation, free
   * with recursive buddy merging, and splitting higher-order blocks to
   * satisfy smaller requests.
   *
   * The allocator does not own its metadata arrays. The architecture layer
   * places them in memory and passes pointers at initialization. A function
   * pointer converts addresses to dereferenceable pointers for embedded
   * free-list node access.
   */
  template<typename Address>
  class BuddyAllocator {
    public:
      /**
       * @brief Function type that converts a address to a pointer to a
       *        @ref BuddyFreeNode embedded in the free block.
       */
      using ToNodeFn = BuddyFreeNode<Address>* (*)(Address);

      /**
       * @brief Initializes the buddy allocator.
       * @param maydayHandler Pointer to a concrete implementation of
       *                      @ref IMaydayHandler.
       * @param blockOrders Per-block order/status array provided by the
       *                    architecture layer.
       * @param blockCount Total number of blocks under management.
       * @param blockSize Size of each block in bytes.
       * @param toNode @ref ToNodeFn to convert an address to a
       *               @ref BuddyFreeNode pointer.
       */
      void Initialize(
        IMaydayHandler* maydayHandler,
        UInt8* blockOrders,
        UInt32 blockCount,
        Address blockSize,
        ToNodeFn toNode
      ) {
        _maydayHandler = maydayHandler;
        _blockOrders = blockOrders;
        _blockCount = blockCount;
        _blockSize = blockSize;
        _toNode = toNode;

        for (
          UInt8 order = 0;
          order <= MaxBuddyOrder;
          ++order
        ) {
          _freeLists[order] = {};
        }
      }

      /**
       * @brief Allocates a single block by finding the smallest
       *        available order and splitting down to order 0.
       * @param topDown When `true`, pick from the head (highest address);
       *                otherwise pick from the tail (lowest address).
       * @return The address of the allocated block, or `0` on failure.
       */
      Address AllocateBlock(bool topDown) {
        for (
          UInt8 order = 0;
          order <= MaxBuddyOrder;
          ++order
        ) {
          FreeMemoryBlockListHead<Address>& head = _freeLists[order];

          if (head.Count == 0) {
            continue;
          }

          Address blockAddress = topDown ? head.First : head.Last;

          if (blockAddress == 0) {
            continue;
          }

          RemoveFromFreeList(blockAddress, order);

          // split down to order 0
          UInt8 currentOrder = order;

          while (currentOrder > 0) {
            --currentOrder;

            Address buddyAddress
              = blockAddress
              + (
                static_cast<Address>(1u << currentOrder) * _blockSize
              );
            UInt32 buddyIndex
              = static_cast<UInt32>(buddyAddress / _blockSize);

            _blockOrders[buddyIndex] = currentOrder;

            UInt32 buddySpan = 1u << currentOrder;

            for (
              UInt32 offset = 1;
              offset < buddySpan && (buddyIndex + offset) < _blockCount;
              ++offset
            ) {
              _blockOrders[buddyIndex + offset] = OrderSentinel;
            }

            InsertIntoFreeList(buddyAddress, currentOrder);
          }

          UInt32 blockIndex = static_cast<UInt32>(blockAddress / _blockSize);

          _blockOrders[blockIndex] = OrderAllocated;

          return blockAddress;
        }

        return 0;
      }

      /**
       * @brief Allocates a contiguous run of blocks by finding a
       *        sufficiently large order and splitting down.
       * @param blocksNeeded Number of contiguous blocks required.
       *                     Must be `> 0`.
       * @param topDown When `true`, pick from the head (highest address);
       *                otherwise pick from the tail (lowest address).
       * @return The address of the first allocated block, or `0` on failure.
       *         On success, exactly @p blocksNeeded blocks are marked
       *         @ref OrderAllocated; any excess from rounding up to a power
       *         of 2 is returned to the free lists.
       */
      Address AllocateBlocks(UInt32 blocksNeeded, bool topDown) {
        if (blocksNeeded <= 1) {
          return AllocateBlock(topDown);
        }

        UInt8 requiredOrder = CeilLog2(blocksNeeded);

        for (
          UInt8 order = requiredOrder;
          order <= MaxBuddyOrder;
          ++order
        ) {
          FreeMemoryBlockListHead<Address>& head = _freeLists[order];

          if (head.Count == 0) {
            continue;
          }

          Address blockAddress = topDown ? head.First : head.Last;

          if (blockAddress == 0) {
            continue;
          }

          RemoveFromFreeList(blockAddress, order);

          // split down to requiredOrder
          UInt8 currentOrder = order;

          while (currentOrder > requiredOrder) {
            --currentOrder;

            Address buddyAddress
              = blockAddress
              + (
                static_cast<Address>(1u << currentOrder) * _blockSize
              );
            UInt32 buddyIndex = static_cast<UInt32>(buddyAddress / _blockSize);

            _blockOrders[buddyIndex] = currentOrder;

            UInt32 buddySpan = 1u << currentOrder;

            for (
              UInt32 offset = 1;
              offset < buddySpan && (buddyIndex + offset) < _blockCount;
              ++offset
            ) {
              _blockOrders[buddyIndex + offset] = OrderSentinel;
            }

            InsertIntoFreeList(buddyAddress, currentOrder);
          }

          UInt32 baseIndex = static_cast<UInt32>(blockAddress / _blockSize);
          UInt32 allocatedBlocks = 1u << requiredOrder;

          for (
            UInt32 blockIndex = 0;
            blockIndex < blocksNeeded;
            ++blockIndex
          ) {
            _blockOrders[baseIndex + blockIndex] = OrderAllocated;
          }

          // return excess blocks to free lists
          UInt32 excessBlocks = allocatedBlocks - blocksNeeded;

          if (excessBlocks > 0) {
            ReturnBlocksToFreeLists(
              baseIndex + blocksNeeded,
              excessBlocks
            );
          }

          return blockAddress;
        }

        return 0;
      }

      /**
       * @brief Allocates a single block whose address is below @p limit.
       * @param limit Exclusive upper bound on the block address.
       * @return The address of the allocated block, or `0` if none is available
       *         below @p limit.
       */
      Address AllocateBlockBelow(Address limit) {
        UInt32 maxBlock = static_cast<UInt32>(limit / _blockSize);

        if (maxBlock == 0) {
          return 0;
        }

        if (maxBlock > _blockCount) {
          maxBlock = _blockCount;
        }

        for (
          UInt8 order = 0;
          order <= MaxBuddyOrder;
          ++order
        ) {
          FreeMemoryBlockListHead<Address>& head = _freeLists[order];

          if (head.Count == 0) {
            continue;
          }

          // walk from tail (lowest address) to find a block below limit
          Address address = head.Last;

          while (address != 0) {
            UInt32 blockIndex
              = static_cast<UInt32>(address / _blockSize);

            if (blockIndex < maxBlock) {
              RemoveFromFreeList(address, order);

              // split down to order 0
              UInt8 currentOrder = order;

              while (currentOrder > 0) {
                --currentOrder;

                Address buddyAddress
                  = address
                  + (
                    static_cast<Address>(1u << currentOrder) *
                    _blockSize
                  );
                UInt32 buddyIndex = static_cast<UInt32>(
                  buddyAddress / _blockSize
                );

                _blockOrders[buddyIndex] = currentOrder;

                UInt32 buddySpan = 1u << currentOrder;

                for (
                  UInt32 offset = 1;
                  offset < buddySpan && (buddyIndex + offset) < _blockCount;
                  ++offset
                ) {
                  _blockOrders[buddyIndex + offset] = OrderSentinel;
                }

                InsertIntoFreeList(buddyAddress, currentOrder);
              }

              _blockOrders[blockIndex] = OrderAllocated;

              return address;
            }

            // walk toward lower addresses
            Address nextAddress = _toNode(address)->Next;

            if (
              nextAddress != 0 && (
                nextAddress % _blockSize != 0 ||
                nextAddress / _blockSize >= _blockCount
              )
            ) {
              _maydayHandler->Handle(
                "Corrupted buddy free list (AllocateBelow walk)"
              );
            }

            address = nextAddress;
          }
        }

        return 0;
      }

      /**
       * @brief Frees a single block and recursively merges with its
       *        buddy.
       * @param blockIndex The block index to free.
       */
      void BuddyFree(UInt32 blockIndex) {
        UInt8 order = 0;
        UInt32 currentIndex = blockIndex;

        while (order < MaxBuddyOrder) {
          UInt32 buddyIndex = currentIndex ^ (1u << order);

          if (
            buddyIndex >= _blockCount ||
            _blockOrders[buddyIndex] != order
          ) {
            break;
          }

          RemoveFromFreeList(
            static_cast<Address>(buddyIndex) * _blockSize,
            order
          );

          _blockOrders[buddyIndex] = OrderSentinel;

          currentIndex = currentIndex & ~(1u << order);
          ++order;
        }

        _blockOrders[currentIndex] = order;

        UInt32 blockSpan = 1u << order;

        for (
          UInt32 i = 1;
          i < blockSpan && (currentIndex + i) < _blockCount;
          ++i
        ) {
          _blockOrders[currentIndex + i] = OrderSentinel;
        }

        InsertIntoFreeList(
          static_cast<Address>(currentIndex) * _blockSize,
          order
        );
      }

      /**
       * @brief Removes a free block from its order's free list.
       * @param address Base address of the block.
       * @param order The buddy order of the block.
       */
      void RemoveFromFreeList(Address address, UInt8 order) {
        BuddyFreeNode<Address>* node = _toNode(address);
        FreeMemoryBlockListHead<Address>& head = _freeLists[order];
        Address previous = node->Previous;
        Address next = node->Next;

        if (
          next != 0 && (
            next % _blockSize != 0 ||
            next / _blockSize >= _blockCount
          )
        ) {
          _maydayHandler->Handle(
            "Corrupted buddy free list (bad Next pointer)"
          );
        }

        if (
          previous != 0 && (
            previous % _blockSize != 0 ||
            previous / _blockSize >= _blockCount
          )
        ) {
          _maydayHandler->Handle(
            "Corrupted buddy free list (bad Previous pointer)"
          );
        }

        if (previous != 0) {
          _toNode(previous)->Next = next;
        } else {
          head.First = next;
        }

        if (next != 0) {
          _toNode(next)->Previous = previous;
        } else {
          head.Last = previous;
        }

        --head.Count;
      }

      /**
       * @brief Inserts a free block into its order's free list,
       *        maintaining descending address sort order.
       * @param address Base address of the block.
       * @param order The buddy order of the block.
       */
      void InsertIntoFreeList(Address address, UInt8 order) {
        BuddyFreeNode<Address>* node = _toNode(address);
        FreeMemoryBlockListHead<Address>& head = _freeLists[order];

        if (head.First == 0) {
          node->Next = 0;
          node->Previous = 0;

          head.First = address;
          head.Last = address;
          ++head.Count;

          return;
        }

        // walk from head (highest) to find insertion point
        Address current = head.First;

        while (
          current != 0 &&
          current > address
        ) {
          Address next = _toNode(current)->Next;

          if (
            next != 0 && (
              next % _blockSize != 0 ||
              next / _blockSize >= _blockCount
            )
          ) {
            _maydayHandler->Handle(
              "Corrupted buddy free list (bad Next during insert)"
            );
          }

          if (
            next == 0 ||
            next <= address
          ) {
            BuddyFreeNode<Address>* currentNode = _toNode(current);

            node->Previous = current;
            node->Next = currentNode->Next;

            if (currentNode->Next != 0) {
              _toNode(currentNode->Next)->Previous = address;
            } else {
              head.Last = address;
            }

            currentNode->Next = address;
            ++head.Count;

            return;
          }

          current = next;
        }

        // address is the highest — insert at head
        node->Previous = 0;
        node->Next = head.First;

        if (head.First != 0) {
          _toNode(head.First)->Previous = address;
        }

        head.First = address;

        if (head.Last == 0) {
          head.Last = address;
        }

        ++head.Count;
      }

      /**
       * @brief Decomposes a contiguous run of blocks into maximal
       *        power-of-2 aligned buddy chunks and inserts each into
       *        the appropriate free list.
       * @param startBlock Starting block index.
       * @param count Number of contiguous blocks.
       */
      void ReturnBlocksToFreeLists(UInt32 startBlock, UInt32 count) {
        UInt32 position = startBlock;
        UInt32 remaining = count;

        while (remaining > 0) {
          UInt8 order = 0;

          while (
            order < MaxBuddyOrder &&
            (1u << (order + 1)) <= remaining &&
            ((position & ((1u << (order + 1)) - 1)) == 0)
          ) {
            ++order;
          }

          UInt32 blockSpan = 1u << order;

          _blockOrders[position] = order;

          for (
            UInt32 blockIndex = 1;
            blockIndex < blockSpan;
            ++blockIndex
          ) {
            _blockOrders[position + blockIndex] = OrderSentinel;
          }

          InsertIntoFreeList(
            static_cast<Address>(position) * _blockSize,
            order
          );

          position += blockSpan;
          remaining -= blockSpan;
        }
      }

      /**
       * @brief Marks a region as @ref OrderReserved in the block order array.
       * @param address Base address of the region.
       * @param sizeInBytes Size of the region in bytes.
       */
      void MarkReserved(
        Address address,
        Address sizeInBytes
      ) {
        UInt32 startBlock
          = static_cast<UInt32>(
              (address / _blockSize)
            );
        UInt32 endBlock
          = static_cast<UInt32>(
              ((address + sizeInBytes + _blockSize - 1) / _blockSize)
            );

        if (endBlock > _blockCount) {
          endBlock = _blockCount;
        }

        for (
          UInt32 blockIndex = startBlock;
          blockIndex < endBlock;
          ++blockIndex
        ) {
          _blockOrders[blockIndex] = OrderReserved;
        }
      }

      /**
       * @brief Gets the per-order free list array.
       * @return Pointer to a @ref FreeMemoryBlockListHead array of length
       *         @ref MaxBuddyOrder + 1.
       */
      FreeMemoryBlockListHead<Address>* GetFreeLists() {
        return _freeLists;
      }

      /**
       * @brief Gets the per-order free list array as a `const`.
       * @return `const` pointer to a @ref FreeMemoryBlockListHead array of length
       *         @ref MaxBuddyOrder + 1.
       */
      const FreeMemoryBlockListHead<Address>* GetFreeLists() const {
        return _freeLists;
      }

      /**
       * @brief Gets the per-block order/status array.
       * @return Pointer to a `UInt8` array of length @ref _blockCount.
       */
      UInt8* GetBlockOrders() {
        return _blockOrders;
      }

      /**
       * @brief Gets the block count.
       * @return The total number of blocks under management.
       */
      UInt32 GetBlockCount() const {
        return _blockCount;
      }

      /**
       * @brief Gets the block size.
       * @return The size of each block in bytes.
       */
      Address GetBlockSize() const {
        return _blockSize;
      }

    private:
      /**
       * @brief Pointer to a concrete implementation of @ref IMaydayHandler.
       */
      IMaydayHandler* _maydayHandler = nullptr;

      /**
       * @brief Per-block order/status array provided by the architecture
       *        layer.
       *
       * Length is @ref _blockCount. Each element is either a valid buddy
       * order (`0` to @ref MaxBuddyOrder), or one of the sentinels
       * @ref OrderAllocated, @ref OrderReserved, or @ref OrderSentinel.
       */
      UInt8* _blockOrders = nullptr;

      /**
        * @brief Per-order free list array.
       */
      FreeMemoryBlockListHead<Address> _freeLists[MaxBuddyOrder + 1] = {};

      /**
       * @brief Total number of blocks under management.
       */
      UInt32 _blockCount = 0;

      /**
       * @brief Size of each block in bytes.
       */
      Address _blockSize = 0;

      /**
       * @brief @ref ToNodeFn pointer to convert an address to a
       *        pointer to a @ref BuddyFreeNode embedded in the free block.
       */
      ToNodeFn _toNode = nullptr;
  };
}
