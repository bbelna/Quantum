/**
 * @file Kernel/Concurrency/HybridCFSBitmapScheduler.cpp
 * @brief Implements @ref @QKrnl::Concurrency::HybridCFSBitmapScheduler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "KernelLog.hpp"
#include "HybridCFSBitmapScheduler.hpp"

namespace Quantum::Kernel::Concurrency {
  HybridCFSBitmapScheduler::HybridCFSBitmapScheduler() :
    _heads {},
    _tails {},
    _bitmap(0),
    _cfsQueue(),
    _totalCount(0),
    _sleepQueueHead(nullptr),
    _sleepQueueTail(nullptr),
    _sleepingCount(0),
    _tickCount(0),
    _higherPriorityReady(false),
    _lock(),
    _contextSwitches(0),
    _idleTicks(0),
    _rtTicks(0),
    _cfsTicks(0),
    _sleepWakeups(0),
    _preemptions(0),
    _voluntaryYields(0)
  {
  }

  void HybridCFSBitmapScheduler::Enqueue(Thread* thread) {
    if (!thread) {
      return;
    } else {
      _lock.Acquire();

      if (_isCFS(thread)) {
        _cfsQueue.Insert(thread);
      } else {
        _rtEnqueue(thread);
      }

      _totalCount++;

      _lock.Release();
    }
  }

  void HybridCFSBitmapScheduler::EnqueueFront(Thread* thread) {
    if (!thread) {
      return;
    } else {
      _lock.Acquire();

      if (_isCFS(thread)) {
        _cfsQueue.Insert(thread);  // vruntime determines position
      } else {
        _rtEnqueueFront(thread);
      }

      _totalCount++;

      _lock.Release();
    }
  }

  Thread* HybridCFSBitmapScheduler::Dequeue() {
    _lock.Acquire();

    // RT threads always take precedence
    Thread* thread = _rtDequeue();

    if (!thread) {
      thread = _cfsQueue.RemoveMin();
    }

    if (thread) {
      _totalCount--;
    }

    _lock.Release();

    return thread;
  }

  bool HybridCFSBitmapScheduler::Remove(Thread* thread) {
    if (thread) {
      _lock.Acquire();

      bool removed;

      if (_isCFS(thread)) {
        removed = _cfsQueue.Remove(thread);
      } else {
        removed = _rtRemove(thread);
      }

      if (removed) {
        _totalCount--;
      }

      _lock.Release();

      return removed;
    } else {
      KLOG_WARNING(
        "HybridCFSBitmapScheduler::Remove called with thread=nullptr"
      );

      return false;
    }
  }

  bool HybridCFSBitmapScheduler::IsEmpty() const {
    return _totalCount == 0;
  }

  Size HybridCFSBitmapScheduler::Count() const {
    return _totalCount;
  }

  void HybridCFSBitmapScheduler::ChargeVirtualRuntime(
    Thread* thread,
    UInt64 ticks
  ) {
    if (!_isCFS(thread)) {
      return;
    } else {
      UInt8 level = Enum::ToBase(thread->BasePriority);
      UInt16 weight = CFSWeightTable[level];

      thread->VirtualRuntime += (ticks * VRuntimeScale) / weight;
    }
  }

  void HybridCFSBitmapScheduler::ClampVirtualRuntime(Thread* thread) {
    if (!_isCFS(thread)) {
      return;
    } else {
      UInt64 minVruntime = _cfsQueue.GetMinVirtualRuntime();
      UInt64 floor = (minVruntime >= CFSWakeupBonus)
        ? (minVruntime - CFSWakeupBonus)
        : 0;

      if (thread->VirtualRuntime < floor) {
        thread->VirtualRuntime = floor;
      }
    }
  }

  void HybridCFSBitmapScheduler::InitializeVirtualRuntime(Thread* thread) {
    if (!_isCFS(thread)) {
      return;
    } else {
      thread->VirtualRuntime = _cfsQueue.GetMinVirtualRuntime();
    }
  }

  UInt8 HybridCFSBitmapScheduler::ComputeQuantum(Thread* thread) const {
    if (_isCFS(thread)) {
      Size cfsCount = _cfsQueue.Count();

      if (cfsCount == 0) {
        cfsCount = 1;
      }

      UInt64 quantum = CFSTargetLatency / cfsCount;

      if (quantum < CFSMinGranularity) {
        quantum = CFSMinGranularity;
      }

      if (quantum > 255) {
        quantum = 255;
      }

      return static_cast<UInt8>(quantum);
    } else {
      // RT thread
      UInt8 level = Enum::ToBase(thread->BasePriority);

      if (level < RealTimeMinLevel) {
        return CFSMinGranularity;
      } else {
        UInt8 index = level - RealTimeMinLevel;

        if (index >= 16) {
          index = 15;
        }

        return RTQuantumTable[index];
      }
    }
  }

  UInt64 HybridCFSBitmapScheduler::GetMinVirtualRuntime() const {
    return _cfsQueue.GetMinVirtualRuntime();
  }

  void HybridCFSBitmapScheduler::Sleep(
    Thread* thread,
    UInt64 ticks
  ) {
    if (!thread) {
      return;
    } else {
      _lock.Acquire();

      thread->SleepTicksRemaining = ticks;
      thread->State = ThreadState::Sleeping;

      // remove from run queue (internal, no lock re-acquire)
      if (thread->InSchedulerQueue) {
        if (_isCFS(thread)) {
          _cfsQueue.Remove(thread);
        } else {
          _rtRemove(thread);
        }

        _totalCount--;
      }

      if (ticks > 0) {
        thread->SleepWakeTime = _tickCount + ticks;

        _insertSleepQueue(thread);
      } else {
        thread->SleepWakeTime = 0;
        thread->SchedulerNext = nullptr;
        thread->SchedulerPrevious = nullptr;
      }

      _lock.Release();
    }
  }

  void HybridCFSBitmapScheduler::Suspend(Thread* thread) {
    if (!thread) {
      return;
    } else {
      _lock.Acquire();

      if (thread->State == ThreadState::Ready) {
        if (_isCFS(thread)) {
          _cfsQueue.Remove(thread);
        } else {
          _rtRemove(thread);
        }

        _totalCount--;
      }

      thread->State = ThreadState::Suspended;

      _lock.Release();
    }
  }

  void HybridCFSBitmapScheduler::Resume(Thread* thread) {
    if (!thread) {
      return;
    } else {
      _lock.Acquire();

      if (
        thread->State == ThreadState::Suspended ||
        thread->State == ThreadState::Sleeping
      ) {
        if (thread->State == ThreadState::Sleeping) {
          if (thread->SleepWakeTime > 0) {
            _removeSleepQueue(thread);
          }

          thread->SleepWakeTime = 0;
        }

        thread->State = ThreadState::Ready;
        thread->SleepTicksRemaining = 0;

        if (_isCFS(thread)) {
          ClampVirtualRuntime(thread);

          _cfsQueue.Insert(thread);
        } else {
          _rtEnqueueFront(thread);

          // RT thread resumed: force preemption on the next tick so
          // IPC receivers wake promptly instead of waiting for the
          // current thread's quantum to expire
          _higherPriorityReady = true;
        }

        _totalCount++;
      }

      _lock.Release();
    }
  }

  Size HybridCFSBitmapScheduler::SleepingCount() const {
    return _sleepingCount;
  }

  Thread* HybridCFSBitmapScheduler::Tick(Thread* current) {
    _lock.Acquire();

    _tickCount++;

    // wake timed sleepers whose deadline has passed
    _wakeSleepers(current);

    // periodically rebase vruntimes to prevent overflow
    UInt64 minVruntime = _cfsQueue.GetMinVirtualRuntime();

    if (minVruntime >= VirtualRuntimeRebaseThreshold) {
      _cfsQueue.RebaseVirtualRuntimes(minVruntime);

      // also rebase the currently running thread if it is CFS
      if (current && _isCFS(current)) {
        if (current->VirtualRuntime >= minVruntime) {
          current->VirtualRuntime -= minVruntime;
        } else {
          current->VirtualRuntime = 0;
        }
      }
    }

    // charge vruntime and manage quantum for current thread
    if (
      current &&
      current->State == ThreadState::Running
    ) {
      current->TotalCPUTicks++;

      if (_isCFS(current)) {
        ChargeVirtualRuntime(current, 1);
        _cfsTicks++;
      } else {
        _rtTicks++;
      }

      if (current->QuantumRemaining > 0) {
        current->QuantumRemaining--;
      }

      if (current->QuantumRemaining > 0 && !_higherPriorityReady) {
        _lock.Release();

        return current;  // keep running
      }

      _preemptions++;

      // quantum expired or preempted: re-enqueue current
      current->State = ThreadState::Ready;

      if (_isCFS(current)) {
        _cfsQueue.Insert(current);
      } else {
        _rtEnqueue(current);
      }

      _totalCount++;
    }

    _higherPriorityReady = false;

    // pick the next thread
    Thread* next = _rtDequeue();

    if (!next) {
      next = _cfsQueue.RemoveMin();
    }

    if (next) {
      _totalCount--;
    }

    _lock.Release();

    if (next) {
      next->State = ThreadState::Running;
      next->QuantumRemaining = ComputeQuantum(next);
    }

    return next; // nullptr means run idle thread
  }

  Thread* HybridCFSBitmapScheduler::Yield(Thread* current) {
    if (!current) {
      return nullptr;
    } else {
      _lock.Acquire();

      _voluntaryYields++;

      // charge partial vruntime for ticks consumed this quantum
      if (_isCFS(current)) {
        UInt8 quantumUsed
          = ComputeQuantum(current)
          - current->QuantumRemaining;

        if (quantumUsed == 0) {
          quantumUsed = 1;  // charge at least 1 tick
        }

        ChargeVirtualRuntime(current, quantumUsed);
      }

      current->State = ThreadState::Ready;

      if (_isCFS(current)) {
        _cfsQueue.Insert(current);
      } else {
        _rtEnqueue(current);
      }

      _totalCount++;

      // pick the next thread
      Thread* next = _rtDequeue();

      if (!next) {
        next = _cfsQueue.RemoveMin();
      }

      if (next) {
        _totalCount--;
      }

      _lock.Release();

      if (next) {
        next->State = ThreadState::Running;
        next->QuantumRemaining = ComputeQuantum(next);
      }

      return next;
    }
  }

  bool HybridCFSBitmapScheduler::_isCFS(const Thread* thread) {
    return Enum::ToBase(thread->BasePriority) < RealTimeMinLevel;
  }

  void HybridCFSBitmapScheduler::_rtEnqueue(Thread* thread) {
    UInt8 level = Enum::ToBase(thread->BasePriority);

    if (level >= PriorityLevelCount) {
      level = PriorityLevelCount - 1;
    }

    thread->SchedulerNext = nullptr;
    thread->SchedulerPrevious = _tails[level];

    if (_tails[level]) {
      _tails[level]->SchedulerNext = thread;
    } else {
      _heads[level] = thread;
    }

    _tails[level] = thread;
    _bitmap |= (1u << level);

    thread->InSchedulerQueue = true;
  }

  void HybridCFSBitmapScheduler::_rtEnqueueFront(Thread* thread) {
    UInt8 level = Enum::ToBase(thread->BasePriority);

    if (level >= PriorityLevelCount) {
      level = PriorityLevelCount - 1;
    }

    thread->SchedulerPrevious = nullptr;
    thread->SchedulerNext = _heads[level];

    if (_heads[level]) {
      _heads[level]->SchedulerPrevious = thread;
    } else {
      _tails[level] = thread;
    }

    _heads[level] = thread;
    _bitmap |= (1u << level);

    thread->InSchedulerQueue = true;
  }

  Thread* HybridCFSBitmapScheduler::_rtDequeue() {
    UInt32 rtMask = _bitmap & 0xFFFF0000u;

    if (rtMask == 0) {
      return nullptr;
    } else {
      UInt8 level = 31 - static_cast<UInt8>(__builtin_clz(rtMask));
      Thread* thread = _heads[level];

      _heads[level] = thread->SchedulerNext;

      if (_heads[level]) {
        _heads[level]->SchedulerPrevious = nullptr;
      } else {
        _tails[level] = nullptr;
        _bitmap &= ~(1u << level);
      }

      thread->SchedulerNext = nullptr;
      thread->SchedulerPrevious = nullptr;
      thread->InSchedulerQueue = false;

      return thread;
    }
  }

  bool HybridCFSBitmapScheduler::_rtRemove(Thread* thread) {
    if (!thread->InSchedulerQueue) {
      return false;
    } else {
      UInt8 level = Enum::ToBase(thread->BasePriority);

      if (level >= PriorityLevelCount) {
        level = PriorityLevelCount - 1;
      }

      if (thread->SchedulerPrevious) {
        thread->SchedulerPrevious->SchedulerNext = thread->SchedulerNext;
      } else {
        _heads[level] = thread->SchedulerNext;
      }

      if (thread->SchedulerNext) {
        thread->SchedulerNext->SchedulerPrevious = thread->SchedulerPrevious;
      } else {
        _tails[level] = thread->SchedulerPrevious;
      }

      if (!_heads[level]) _bitmap &= ~(1u << level);

      thread->SchedulerNext = nullptr;
      thread->SchedulerPrevious = nullptr;
      thread->InSchedulerQueue = false;

      return true;
    }
  }

  bool HybridCFSBitmapScheduler::_rtHasReady() const {
    return (_bitmap & 0xFFFF0000u) != 0;
  }

  void HybridCFSBitmapScheduler::_insertSleepQueue(Thread* thread) {
    Thread* current = _sleepQueueTail;

    while (
      current &&
      current->SleepWakeTime > thread->SleepWakeTime
    ) {
      current = current->SchedulerPrevious;
    }

    if (!current) {
      thread->SchedulerNext = _sleepQueueHead;
      thread->SchedulerPrevious = nullptr;

      if (_sleepQueueHead) {
        _sleepQueueHead->SchedulerPrevious = thread;
      } else {
        _sleepQueueTail = thread;
      }

      _sleepQueueHead = thread;
    } else {
      thread->SchedulerNext = current->SchedulerNext;
      thread->SchedulerPrevious = current;

      if (current->SchedulerNext) {
        current->SchedulerNext->SchedulerPrevious = thread;
      } else {
        _sleepQueueTail = thread;
      }

      current->SchedulerNext = thread;
    }

    _sleepingCount++;
  }

  void HybridCFSBitmapScheduler::_removeSleepQueue(Thread* thread) {
    if (thread->SchedulerPrevious) {
      thread->SchedulerPrevious->SchedulerNext = thread->SchedulerNext;
    } else {
      _sleepQueueHead = thread->SchedulerNext;
    }

    if (thread->SchedulerNext) {
      thread->SchedulerNext->SchedulerPrevious = thread->SchedulerPrevious;
    } else {
      _sleepQueueTail = thread->SchedulerPrevious;
    }

    thread->SchedulerNext = nullptr;
    thread->SchedulerPrevious = nullptr;

    if (_sleepingCount > 0) {
      _sleepingCount--;
    }
  }

  void HybridCFSBitmapScheduler::RecordIdleTick() {
    _idleTicks++;
  }

  void HybridCFSBitmapScheduler::RecordContextSwitch() {
    _contextSwitches++;
  }

  void HybridCFSBitmapScheduler::GetStats(SchedulerStats* stats) const {
    stats->TotalTicks = _tickCount;
    stats->ContextSwitches = _contextSwitches;
    stats->IdleTicks = _idleTicks;
    stats->RTTicks = _rtTicks;
    stats->CFSTicks = _cfsTicks;
    stats->SleepWakeups = _sleepWakeups;
    stats->Preemptions = _preemptions;
    stats->VoluntaryYields = _voluntaryYields;
    stats->ReadyCount = static_cast<UInt32>(_totalCount);
    stats->SleepingCount = static_cast<UInt32>(_sleepingCount);
  }

  void HybridCFSBitmapScheduler::_wakeSleepers(Thread* current) {
    while (
      _sleepQueueHead &&
      _sleepQueueHead->SleepWakeTime <= _tickCount
    ) {
      Thread* thread = _sleepQueueHead;

      _sleepQueueHead = thread->SchedulerNext;

      if (_sleepQueueHead) {
        _sleepQueueHead->SchedulerPrevious = nullptr;
      } else {
        _sleepQueueTail = nullptr;
      }

      thread->SchedulerNext = nullptr;
      thread->SchedulerPrevious = nullptr;
      thread->SleepWakeTime = 0;
      thread->SleepTicksRemaining = 0;
      thread->State = ThreadState::Ready;

      _sleepingCount--;
      _sleepWakeups++;

      // CFS: clamp vruntime on wake for fairness
      if (_isCFS(thread)) {
        ClampVirtualRuntime(thread);
        _cfsQueue.Insert(thread);
      } else {
        _rtEnqueue(thread);
      }

      _totalCount++;

      // check if woken thread should preempt current
      if (current && !_isCFS(thread)) {
        if (Enum::ToBase(thread->BasePriority) > Enum::ToBase(current->BasePriority)) {
          _higherPriorityReady = true;
        }
      } else if (current && !_isCFS(current) && _isCFS(thread)) {
        // CFS thread can't preempt RT thread
      } else if (current && _isCFS(current) && !_isCFS(thread)) {
        // RT thread preempts CFS thread
        _higherPriorityReady = true;
      }
    }
  }
}
