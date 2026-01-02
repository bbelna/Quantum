/**
 * @file System/Kernel/WaitQueue.cpp
 * @brief Simple wait queue for blocking threads.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "WaitQueue.hpp"

namespace Quantum::System::Kernel {
  void WaitQueue::Initialize() {
    _lock.Initialize();
    _head = nullptr;
    _tail = nullptr;
  }

  void WaitQueue::EnqueueCurrent() {
    Thread::ControlBlock* thread = Thread::GetCurrent();

    if (thread == nullptr) {
      return;
    }

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_lock);

      thread->waitNext = nullptr;
      thread->state = Thread::State::Blocked;

      if (_tail == nullptr) {
        _head = thread;
        _tail = thread;
      } else {
        _tail->waitNext = thread;
        _tail = thread;
      }
    }

    Thread::Yield();
  }

  bool WaitQueue::WaitTicks(UInt32 ticks) {
    Thread::ControlBlock* thread = Thread::GetCurrent();

    if (thread == nullptr) {
      return false;
    }

    if (ticks == 0) {
      return false;
    }

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_lock);

      thread->waitNext = nullptr;
      thread->state = Thread::State::Blocked;

      if (_tail == nullptr) {
        _head = thread;
        _tail = thread;
      } else {
        _tail->waitNext = thread;
        _tail = thread;
      }
    }

    Thread::SleepTicks(ticks);

    bool removed = false;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_lock);
      Thread::ControlBlock* prev = nullptr;
      Thread::ControlBlock* current = _head;

      while (current) {
        if (current == thread) {
          if (prev) {
            prev->waitNext = current->waitNext;
          } else {
            _head = current->waitNext;
          }

          if (_tail == current) {
            _tail = prev;
          }

          current->waitNext = nullptr;
          removed = true;

          break;
        }

        prev = current;
        current = current->waitNext;
      }
    }

    return !removed;
  }

  bool WaitQueue::WakeOne() {
    Thread::ControlBlock* thread = nullptr;

    {
      Sync::ScopedLock<Sync::SpinLock> guard(_lock);

      if (_head == nullptr) {
        return false;
      }

      thread = _head;
      _head = thread->waitNext;

      if (_head == nullptr) {
        _tail = nullptr;
      }

      thread->waitNext = nullptr;
    }

    Thread::Wake(thread);

    return true;
  }

  void WaitQueue::WakeAll() {
    while (WakeOne()) {}
  }
}
