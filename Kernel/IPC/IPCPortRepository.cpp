/**
 * @file Kernel/IPC/IPCPortRepository.cpp
 * @brief Implements @ref @QKrnl::IPC::IPCPortRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/Process.hpp>
#include <Concurrency/ThreadManager.hpp>
#include <KernelLog.hpp>
#include <Memory/HeapAllocator.hpp>
#include <Memory/ObjectPool.hpp>

#include "IPCPortRepository.hpp"

namespace Quantum::Kernel::IPC {
  IPCPortRepository::IPCPortRepository(
    HeapAllocator& heap,
    ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>& messagePool,
    ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>& portPool,
    ThreadManager& threads
  ) :
    _heap(heap),
    _messagePool(messagePool),
    _portPool(portPool),
    _threads(threads),
    _portIDs(IPC_AUTO_ASSIGN_PORT_ID_START)
  {
  }

  Result<> IPCPortRepository::Save(IPCPort* port) {
    Thread* threadsToWake[THREAD_DEFERRED_WAKE_LIMIT];
    Size wakeCount = 0;

    _lock.Acquire();

    _ports.Append(new LinkedNode<IPCPort*>(port));

    // collect threads waiting for this port ID
    Size writeIndex = 0;

    for (Size i = 0; i < _waiterCount; i++) {
      if (
        _waiterPortIDs[i] == port->ID &&
        wakeCount < THREAD_DEFERRED_WAKE_LIMIT
      ) {
        threadsToWake[wakeCount++] = _waiterThreads[i];
      } else {
        // compact: keep entries that didn't match
        _waiterPortIDs[writeIndex] = _waiterPortIDs[i];
        _waiterThreads[writeIndex] = _waiterThreads[i];
        writeIndex++;
      }
    }

    _waiterCount = writeIndex;

    _lock.Release();

    for (
      Size threadIndex = 0;
      threadIndex < wakeCount;
      threadIndex++
    ) {
      _threads.Resume(threadsToWake[threadIndex]);
    }

    KLOG_DEBUG(
      "Registered port %u for PID %u",
      port->ID,
      port->OwnerProcess
        ? port->OwnerProcess->ID
        : 0
    );

    return Result<>(true);
  }

  Result<> IPCPortRepository::RemoveByID(IPCPortID portID) {
    // collect threads to wake after releasing the lock, calling Resume
    // under a spinlock risks deadlock if the woken thread re-enters IPC
    Thread* threadsToWake[THREAD_DEFERRED_WAKE_LIMIT];
    Size wakeCount = 0;

    _lock.Acquire();

    LinkedNode<IPCPort*>* current = _ports.GetHead();

    while (current) {
      IPCPort* port = current->GetValue();

      if (
        port &&
        port->ID == portID
      ) {
        _ports.Remove(current);

        KLOG_DEBUG(
          "Unregistered port %u for PID %u",
          port->ID,
          port->OwnerProcess
            ? port->OwnerProcess->ID
            : 0
        );

        // collect threads blocked in Receive, they will check the
        // registry after waking and find the port gone
        ThreadWaitNode* waitNode = port->ReceiveQueue.Dequeue();

        while (waitNode) {
          if (
            waitNode->Owner &&
            wakeCount < THREAD_DEFERRED_WAKE_LIMIT
          ) {
            threadsToWake[wakeCount++] = waitNode->Owner;
          }

          waitNode = port->ReceiveQueue.Dequeue();
        }

        // collect threads blocked in Send
        waitNode = port->SendQueue.Dequeue();

        while (waitNode) {
          if (
            waitNode->Owner &&
            wakeCount < THREAD_DEFERRED_WAKE_LIMIT
          ) {
            threadsToWake[wakeCount++] = waitNode->Owner;
          }

          waitNode = port->SendQueue.Dequeue();
        }

        // drain any remaining messages from the port's queue and free
        // their kernel-heap-allocated payloads
        IPCMessage* message = port->MessageQueue.Dequeue();

        while (message) {
          if (message->Payload) {
            _heap.Free(message->Payload);
          }

          _messagePool.Free(message);

          message = port->MessageQueue.Dequeue();
        }

        // do NOT recycle auto-assigned port IDs, a stale waiter that
        // saved the old ID could match a newly allocated port with the
        // same ID, leading to message delivery to the wrong port
        // (safe for alpha: ~20 ports per boot vs 1024 max)

        // not heap allocated, need to explicitly call destructor and free from
        // pool
        port->~IPCPort();
        _portPool.Free(port);

        delete current;

        _lock.Release();

        // resume collected threads now that the lock is released
        for (
          Size threadIndex = 0;
          threadIndex < wakeCount;
          threadIndex++
        ) {
          _threads.Resume(threadsToWake[threadIndex]);
        }

        return Result<>(true);
      }

      current = current->GetNext();
    }

    _lock.Release();

    return Result<>(false);
  }

  IPCPortID IPCPortRepository::AllocateID() {
    _lock.Acquire();

    IPCPortID id;
    bool success = _portIDs.Allocate(id);

    _lock.Release();

    return success
      ? id
      : 0;
  }

  bool IPCPortRepository::WaitForPort(IPCPortID portID) {
    _lock.Acquire();

    // check if port already exists
    LinkedNode<IPCPort*>* current = _ports.GetHead();

    while (current) {
      IPCPort* port = current->GetValue();

      if (port && port->ID == portID) {
        _lock.Release();

        return true;
      }

      current = current->GetNext();
    }

    // port doesn't exist yet, register as waiter
    if (_waiterCount >= MaxPortWaiters) {
      _lock.Release();

      KLOG_ERROR("WaitForPort: waiter table full for port %u", portID);

      return false;
    }

    Thread* thread = _threads.GetCurrent();

    _waiterPortIDs[_waiterCount] = portID;
    _waiterThreads[_waiterCount] = thread;
    _waiterCount++;

    _lock.Release();

    // sleep until Save() wakes us
    _threads.Sleep(thread, 0);

    return true;
  }

  Result<IPCPort*> IPCPortRepository::FindByID(IPCPortID portID) {
    _lock.Acquire();

    LinkedNode<IPCPort*>* current = _ports.GetHead();

    while (current) {
      IPCPort* port = current->GetValue();

      if (
        port &&
        port->ID == portID
      ) {
        _lock.Release();

        return Result<IPCPort*>(
          true,
          port
        );
      }

      current = current->GetNext();
    }

    _lock.Release();

    return Result<IPCPort*>(false);
  }
}
