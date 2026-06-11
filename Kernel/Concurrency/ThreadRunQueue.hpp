/**
 * @file Kernel/Concurrency/ThreadRunQueue.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ThreadRunQueue.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Thread.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Intrusive red-black tree run queue for the @ref Scheduler
   *        time-sharing band.
   *
   * @ref Thread instances are ordered by @ref Thread::VirtualRuntime
   * (ascending), with ties broken by @ref Thread::ID. Node pointers
   * (@ref Thread::RBLeft, @ref Thread::RBRight, @ref Thread::RBParent,
   * @ref Thread::RBNodeColor) are embedded directly in the @ref Thread,
   * so insertion and removal are allocation-free.
   *
   * The leftmost node (lowest virtual runtime) is cached for
   * \f$\mathcal{O}(1)\f$ access. A monotonically advancing
   * @ref _minVirtualRuntime floor tracks the minimum virtual runtime among all
   * runnable threads, used for wake-up clamping.
   *
   * This class is NOT internally synchronized; the caller (@ref Scheduler)
   * must hold its own lock.
   */
  class ThreadRunQueue {
    public:
      /**
       * @brief Constructs an empty @ref ThreadRunQueue.
       */
      ThreadRunQueue() = default;

      /**
       * @brief Inserts a @ref Thread into the @ref ThreadRunQueue.
       * @param thread The @ref Thread to insert.
       * @note @p thread must not already be in the @ref ThreadRunQueue.
       * @note Complexity is \f$\mathcal{O}(\ln n)\f$.
       * 
       * Updates the cached leftmost node and minimum virtual runtime.
       */
      void Insert(Thread* thread);

      /**
       * @brief Removes and returns the @ref Thread with the lowest virtual
       *        runtime.
       * @return The leftmost @ref Thread, or `nullptr` if the @ref RunQueue is
       *         empty.
       * @note Complexity is \f$\mathcal{O}(\ln n)\f$ due to red-black tree
       *       rebalancing.
       */
      Thread* RemoveMin();

      /**
       * @brief Removes a specific @ref Thread from the @ref RunQueue.
       * @param thread Pointer to the @ref Thread to remove from the
       *               @ref RunQueue.
       * @return `true` if the @ref Thread was found and removed; `false`
       *         otherwise.
       * @note Complexity is \f$\mathcal{O}(\ln n)\f$.
       */
      bool Remove(Thread* thread);

      /**
       * @brief Gets the leftmost @ref Thread (the @ref Thread with the lowest
       *        virtual runtime) without removing it.
       * @return The leftmost @ref Thread, or `nullptr` if the @ref RunQueue is
       *         empty.
       * @note Complexity is \f$\mathcal{O}(1)\f$ via the cached leftmost node.
       */
      Thread* PeekMin() const {
        return _leftmost;
      }

      /**
       * @brief Gets the current minimum virtual runtime floor.
       * @return The minimum virtual runtime (monotonically advancing).
       */
      UInt64 GetMinVirtualRuntime() const {
        return _minVirtualRuntime;
      }

      /**
       * @brief Gets the number of @ref Thread instances in the @ref RunQueue.
       * @return The @ref Thread instance count in the @ref RunQueue.
       */
      Size Count() const {
        return _threadCount;
      }

      /**
       * @brief Checks whether the @ref RunQueue is empty.
       * @return `true` if no @ref Thread instances are in the @ref RunQueue;
       *         `false` otherwise.
       * @note Complexity is \f$\mathcal{O}(1)\f$ via the cached root pointer.
       */
      bool IsEmpty() const {
        return _root == nullptr;
      }

      /**
       * @brief Rebases all virtual runtimes by subtracting @p amount from every
       *        node and from @ref _minVirtualRuntime.
       * @param amount The value to subtract.
       *
       * Preserves tree ordering since the same value is subtracted from
       * all keys. Called periodically to prevent virtual runtime overflow.
       */
      void RebaseVirtualRuntimes(UInt64 amount);

    private:
      /**
       * @brief Recursively subtracts @p amount from all @ref Thread nodes'
       *        virtual runtimes.
       * @param node @ref Thread pointer to the current subtree root.
       * @param amount The value to subtract.
       */
      static void _rebaseSubtree(
        Thread* node,
        UInt64 amount
      );

      /**
       * @brief Root of the red-black tree containing all @ref Thread instances
       *        in the @ref RunQueue.
       */
      Thread* _root = nullptr;

      /**
       * @brief Cached leftmost (minimum virtual runtime) node for
       *        \f$\mathcal{O}(1)\f$ access.
       */
      Thread* _leftmost = nullptr;

      /**
       * @brief Number of @ref Thread instances in the @ref RunQueue.
       */
      Size _threadCount = 0;

      /**
       * @brief Monotonically advancing minimum virtual runtime floor.
       */
      UInt64 _minVirtualRuntime = 0;

      /**
       * @brief Compares two @ref Thread instances by
       *        @ref Thread::VirtualRuntime, then by @ref Thread::ID for
       *        tie-breaking.
       * @param a First @ref Thread.
       * @param b Second @ref Thread.
       * @return `true` if @p a should go left of @p b in the tree.
       */
      static bool _isLess(const Thread* a, const Thread* b);

      /**
       * @brief Returns the color of a @ref Thread node.
       * @note `nullptr` is treated as black.
       * @param thread Pointer to a @ref Thread representing the node (may be
       *               `nullptr`).
       * @return `0` (red) or `1` (black).
       */
      static UInt8 _color(const Thread* thread);

      /**
       * @brief Sets a @ref Thread instance's node color.
       * @param thread Pointer to a @ref Thread representing the node (may be
       *               `nullptr`).
       * @param color `0` for red, `1` for black.
       */
      static void _setColor(Thread* thread, UInt8 color);

      /**
       * @brief Finds the leftmost (minimum) node in a subtree.
       * @param node Pointer to a @ref Thread representing the subtree root
       *             (may be `nullptr`).
       * @return Pointer to the leftmost @ref Thread node, or `nullptr` if
       *         @p node is `nullptr`.
       */
      static Thread* _findMin(Thread* node);

      /**
       * @brief Performs a left rotation around the given node.
       * @param x Pointer to a @ref Thread representing the node to rotate
       *          around.
       */
      void _rotateLeft(Thread* x);

      /**
       * @brief Performs a right rotation around the given node.
       * @param x Pointer to a @ref Thread representing the node to rotate
       *          around.
       */
      void _rotateRight(Thread* x);

      /**
       * @brief Replaces subtree rooted at @p u with subtree rooted at @p v.
       * @param u Pointer to a @ref Thread representing the node to replace.
       * @param v Pointer to a @ref Thread representing the replacement subtree
       *          root (may be `nullptr`).
       */
      void _transplant(
        Thread* u,
        Thread* v
      );

      /**
       * @brief Restores red-black properties after insertion.
       * @param z Pointer to a @ref Thread representing the newly inserted node.
       */
      void _insertFixup(Thread* z);

      /**
       * @brief Restores red-black properties after deletion.
       * @param x Pointer to a @ref Thread representing the node that replaced
       *          the deleted node (may be `nullptr`).
       * @param xParent Pointer to a @ref Thread representing the parent of
       *                @p x (used when @p x is `nullptr`).
       */
      void _removeFixup(Thread* x, Thread* xParent);

      /**
       * @brief Removes a node from the tree (internal, does not update
       *        @ref _count or @ref _leftmost).
       * @param z Pointer to a @ref Thread representing the node to remove.
       */
      void _removeNode(Thread* z);

      /**
       * @brief Updates @ref _minVirtualRuntime from the current leftmost node.
       */
      void _updateMinVirtualRuntime();
  };
}
