/**
 * @file Kernel/Concurrency/ThreadRunQueue.cpp
 * @brief Implements @ref @QKrnl::Concurrency::ThreadRunQueue.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ThreadRunQueue.hpp"

namespace Quantum::Kernel::Concurrency {
  void ThreadRunQueue::Insert(Thread* thread) {
    // clear RB node state
    thread->RBLeft = nullptr;
    thread->RBRight = nullptr;
    thread->RBParent = nullptr;
    thread->RBNodeColor = 0;  // new nodes are red
    thread->InSchedulerQueue = true;

    if (!_root) {
      thread->RBNodeColor = 1;  // root is black

      _root = thread;
      _leftmost = thread;
      _threadCount = 1;

      _updateMinVirtualRuntime();

      return;
    }

    // BST insert by vruntime+ID key
    Thread* current = _root;
    Thread* parent = nullptr;
    bool wentLeft = false;

    while (current) {
      parent = current;

      if (
        _isLess(
          thread,
          current
        )
      ) {
        current = current->RBLeft;
        wentLeft = true;
      } else {
        current = current->RBRight;
        wentLeft = false;
      }
    }

    thread->RBParent = parent;

    if (wentLeft) {
      parent->RBLeft = thread;
    } else {
      parent->RBRight = thread;
    }

    _threadCount++;

    _insertFixup(thread);

    // update leftmost if this node is the new minimum
    if (
      !_leftmost ||
      _isLess(
        thread,
        _leftmost
      )
    ) {
      _leftmost = thread;
    }

    _updateMinVirtualRuntime();
  }

  Thread* ThreadRunQueue::RemoveMin() {
    if (_leftmost) {
      Thread* toRemove = _leftmost;

      _removeNode(toRemove);

      _threadCount--;

      toRemove->RBLeft = nullptr;
      toRemove->RBRight = nullptr;
      toRemove->RBParent = nullptr;
      toRemove->RBNodeColor = 1;
      toRemove->InSchedulerQueue = false;

      _leftmost = _findMin(_root);

      _updateMinVirtualRuntime();

      return toRemove;
    } else {
      return nullptr;
    }
  }

  bool ThreadRunQueue::Remove(Thread* thread) {
    if (
      thread &&
      thread->InSchedulerQueue
    ) {
      bool wasLeftmost = thread == _leftmost;

      _removeNode(thread);

      _threadCount--;

      thread->RBLeft = nullptr;
      thread->RBRight = nullptr;
      thread->RBParent = nullptr;
      thread->RBNodeColor = 1;
      thread->InSchedulerQueue = false;

      if (wasLeftmost) {
        _leftmost = _findMin(_root);
      }

      _updateMinVirtualRuntime();

      return true;
    } else {
      return false;
    }
  }

  bool ThreadRunQueue::_isLess(
    const Thread* a,
    const Thread* b
  ) {
    return
        a->VirtualRuntime != b->VirtualRuntime
      ? a->VirtualRuntime < b->VirtualRuntime
      : a->ID < b->ID;
  }

  UInt8 ThreadRunQueue::_color(const Thread* thread) {
    return
        thread
      ? thread->RBNodeColor
      : 1;  // nullptr is black
  }

  void ThreadRunQueue::_setColor(Thread* thread, UInt8 color) {
    if (thread) {
      thread->RBNodeColor = color;
    }
  }

  Thread* ThreadRunQueue::_findMin(Thread* node) {
    if (node) {
      while (node->RBLeft) {
        node = node->RBLeft;
      }

      return node;
    } else {
      return nullptr;
    }
  }

  void ThreadRunQueue::_rotateLeft(Thread* x) {
    Thread* y = x->RBRight;

    x->RBRight = y->RBLeft;

    if (y->RBLeft) {
      y->RBLeft->RBParent = x;
    }

    y->RBParent = x->RBParent;

    if (!x->RBParent) {
      _root = y;
    } else if (x == x->RBParent->RBLeft) {
      x->RBParent->RBLeft = y;
    } else {
      x->RBParent->RBRight = y;
    }

    y->RBLeft = x;
    x->RBParent = y;
  }

  void ThreadRunQueue::_rotateRight(Thread* x) {
    Thread* y = x->RBLeft;

    x->RBLeft = y->RBRight;

    if (y->RBRight) y->RBRight->RBParent = x;

    y->RBParent = x->RBParent;

    if (!x->RBParent) {
      _root = y;
    } else if (x == x->RBParent->RBRight) {
      x->RBParent->RBRight = y;
    } else {
      x->RBParent->RBLeft = y;
    }

    y->RBRight = x;
    x->RBParent = y;
  }

  void ThreadRunQueue::_transplant(Thread* u, Thread* v) {
    if (!u->RBParent) {
      _root = v;
    } else if (u == u->RBParent->RBLeft) {
      u->RBParent->RBLeft = v;
    } else {
      u->RBParent->RBRight = v;
    }

    if (v) {
      v->RBParent = u->RBParent;
    }
  }

  void ThreadRunQueue::_insertFixup(Thread* z) {
    while (
      z->RBParent &&
      z->RBParent->RBNodeColor == 0
    ) {
      Thread* parent = z->RBParent;
      Thread* grandparent = parent->RBParent;

      if (!grandparent) {
        break;
      }

      if (parent == grandparent->RBLeft) {
        Thread* uncle = grandparent->RBRight;

        if (_color(uncle) == 0) {
          parent->RBNodeColor = 1;

          _setColor(
            uncle,
            1
          );

          grandparent->RBNodeColor = 0;

          z = grandparent;
        } else {
          if (z == parent->RBRight) {
            z = parent;

            _rotateLeft(z);

            parent = z->RBParent;
            grandparent = parent->RBParent;
          }

          parent->RBNodeColor = 1;
          grandparent->RBNodeColor = 0;

          _rotateRight(grandparent);
        }
      } else {
        Thread* uncle = grandparent->RBLeft;

        if (_color(uncle) == 0) {
          parent->RBNodeColor = 1;

          _setColor(
            uncle,
            1
          );

          grandparent->RBNodeColor = 0;

          z = grandparent;
        } else {
          if (z == parent->RBLeft) {
            z = parent;

            _rotateRight(z);

            parent = z->RBParent;
            grandparent = parent->RBParent;
          }

          parent->RBNodeColor = 1;
          grandparent->RBNodeColor = 0;

          _rotateLeft(grandparent);
        }
      }
    }

    _root->RBNodeColor = 1;
  }

  void ThreadRunQueue::_removeFixup(
    Thread* x,
    Thread* xParent
  ) {
    while (
      x != _root &&
      _color(x) == 1
    ) {
      if (
        x == (
            xParent
          ? xParent->RBLeft
          : nullptr
        )
      ) {
        Thread* w = xParent->RBRight;

        if (w->RBNodeColor == 0) {
          w->RBNodeColor = 1;
          xParent->RBNodeColor = 0;

          _rotateLeft(xParent);

          w = xParent->RBRight;
        }

        if (
          _color(w->RBLeft) == 1 &&
          _color(w->RBRight) == 1
        ) {
          w->RBNodeColor = 0;

          x = xParent;
          xParent = x->RBParent;
        } else {
          if (_color(w->RBRight) == 1) {
            _setColor(w->RBLeft, 1);

            w->RBNodeColor = 0;

            _rotateRight(w);

            w = xParent->RBRight;
          }

          w->RBNodeColor = xParent->RBNodeColor;
          xParent->RBNodeColor = 1;

          _setColor(
            w->RBRight,
            1
          );
          _rotateLeft(xParent);

          x = _root;
        }
      } else {
        Thread* w = xParent->RBLeft;

        if (w->RBNodeColor == 0) {
          w->RBNodeColor = 1;
          xParent->RBNodeColor = 0;

          _rotateRight(xParent);

          w = xParent->RBLeft;
        }

        if (
          _color(w->RBRight) == 1 &&
          _color(w->RBLeft) == 1
        ) {
          w->RBNodeColor = 0;
          x = xParent;
          xParent = x->RBParent;
        } else {
          if (_color(w->RBLeft) == 1) {
            _setColor(
              w->RBRight,
              1
            );

            w->RBNodeColor = 0;

            _rotateLeft(w);

            w = xParent->RBLeft;
          }

          w->RBNodeColor = xParent->RBNodeColor;
          xParent->RBNodeColor = 1;

          _setColor(
            w->RBLeft,
            1
          );
          _rotateRight(xParent);

          x = _root;
        }
      }
    }

    _setColor(x, 1);
  }

  void ThreadRunQueue::_removeNode(Thread* z) {
    UInt8 originalColor = z->RBNodeColor;
    Thread* y = z;
    Thread* x;
    Thread* xParent;

    if (!z->RBLeft) {
      x = z->RBRight;
      xParent = z->RBParent;

      _transplant(
        z,
        x
      );
    } else if (!z->RBRight) {
      x = z->RBLeft;
      xParent = z->RBParent;

      _transplant(
        z,
        x
      );
    } else {
      y = _findMin(z->RBRight);
      originalColor = y->RBNodeColor;
      x = y->RBRight;

      if (y->RBParent == z) {
        xParent = y;
      } else {
        xParent = y->RBParent;

        _transplant(
          y,
          x
        );

        y->RBRight = z->RBRight;
        z->RBRight->RBParent = y;
      }

      _transplant(
        z,
        y
      );

      y->RBLeft = z->RBLeft;
      z->RBLeft->RBParent = y;
      y->RBNodeColor = z->RBNodeColor;
    }

    if (originalColor == 1) {
      _removeFixup(
        x,
        xParent
      );
    }
  }

  void ThreadRunQueue::RebaseVirtualRuntimes(UInt64 amount) {
    if (
      amount > 0 &&
      _root
    ) {
      _rebaseSubtree(
        _root,
        amount
      );

      if (_minVirtualRuntime >= amount) {
        _minVirtualRuntime -= amount;
      } else {
        _minVirtualRuntime = 0;
      }
    }
  }

  void ThreadRunQueue::_rebaseSubtree(
    Thread* node,
    UInt64 amount
  ) {
    if (node) {
      _rebaseSubtree(
        node->RBLeft,
        amount
      );

      if (node->VirtualRuntime >= amount) {
        node->VirtualRuntime -= amount;
      } else {
        node->VirtualRuntime = 0;
      }

      _rebaseSubtree(
        node->RBRight,
        amount
      );
    }
  }

  void ThreadRunQueue::_updateMinVirtualRuntime() {
    if (_leftmost) {
      if (_leftmost->VirtualRuntime > _minVirtualRuntime) {
        _minVirtualRuntime = _leftmost->VirtualRuntime;
      }
    }
    // if tree is empty, _minVirtualRuntime stays at its last value (monotonic)
  }
}
