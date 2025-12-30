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
    _head = nullptr;
    _tail = nullptr;
  }

  void WaitQueue::EnqueueCurrent() {
    Thread::ControlBlock* thread = Thread::GetCurrent();

    if (thread == nullptr) {
      return;
    }

    thread->waitNext = nullptr;
    thread->state = Thread::State::Blocked;

    if (_tail == nullptr) {
      _head = thread;
      _tail = thread;
    } else {
      _tail->waitNext = thread;
      _tail = thread;
    }

    Thread::Yield();
  }

  bool WaitQueue::WakeOne() {
    if (_head == nullptr) {
      return false;
    }

    Thread::ControlBlock* thread = _head;
    _head = thread->waitNext;

    if (_head == nullptr) {
      _tail = nullptr;
    }

    thread->waitNext = nullptr;

    Thread::Wake(thread);

    return true;
  }

  void WaitQueue::WakeAll() {
    while (WakeOne()) {}
  }
}
