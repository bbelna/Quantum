/**
 * @file Include/Quantum/Structures/Queues.hpp
 * @brief Declares @ref Quantum::Structures::Queues::IntrusiveQueue.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Structures::Queues {
  /**
   * @brief Intrusive doubly-linked FIFO queue with \f$\mathcal{O}(1)\f$
   *        enqueue, dequeue, remove, and count.
   * @tparam T Element type. Must have public `T* Next` and `T* Previous`
   *           pointer members, both initialized to `nullptr` when not queued.
   *
   * Does not allocate memory; elements must embed `T* Next` and `T* Previous`
   * members.
   */
  template<typename T>
  class IntrusiveQueue {
    public:
      /**
       * @brief Creates a new @ref IntrusiveQueue instance.
       */
      IntrusiveQueue() = default;

      /**
       * @brief Adds an item to the tail of the queue.
       * @param item Pointer to the item to enqueue. Must not already be in a
       *             queue.
       */
      void Enqueue(T* item) {
        if (!item) return;

        item->Next = nullptr;
        item->Previous = _tail;

        if (_tail) {
          _tail->Next = item;
        } else {
          _head = item;
        }

        _tail = item;

        ++_count;
      }

      /**
       * @brief Removes and returns the item at the head of the queue.
       * @return Pointer to the dequeued item, or `nullptr` if the queue is
       *         empty.
       */
      T* Dequeue() {
        if (!_head) return nullptr;

        T* item = _head;

        _head = item->Next;

        if (_head) {
          _head->Previous = nullptr;
        } else {
          _tail = nullptr;
        }

        item->Next = nullptr;
        item->Previous = nullptr;

        --_count;

        return item;
      }

      /**
       * @brief Removes a specific item from the queue. Safe to call if the
       *        item has already been removed (no-op in that case).
       * @param item Pointer to the item to remove.
       */
      void Remove(T* item) {
        if (!item) return;

        if (!item->Previous && !item->Next && _head != item) return;

        if (item->Previous) {
          item->Previous->Next = item->Next;
        } else {
          _head = item->Next;
        }

        if (item->Next) {
          item->Next->Previous = item->Previous;
        } else {
          _tail = item->Previous;
        }

        item->Next = nullptr;
        item->Previous = nullptr;

        --_count;
      }

      /**
       * @brief Returns the number of items in the queue.
       * @return The item count.
       */
      Size GetCount() const { return _count; }

      /**
       * @brief Returns the item at the head of the queue without removing it.
       * @return Pointer to the head item, or `nullptr` if empty.
       */
      T* GetHead() const { return _head; }

      /**
       * @brief Checks whether the queue is empty.
       * @return `true` if the queue contains no items; `false` otherwise.
       */
      bool IsEmpty() const { return _count == 0; }

    private:
      /**
       * @brief The head of the queue.
       */
      T* _head = nullptr;

      /**
       * @brief The tail of the queue.
       */
      T* _tail = nullptr;

      /**
       * @brief Number of elements in the queue.
       */
      Size _count = 0;
  };
}
