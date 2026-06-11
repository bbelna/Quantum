/**
 * @file Kernel/IPC/IPCPortService.cpp
 * @brief Implements @ref @QKrnl::IPC::IPCPortService.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/Thread.hpp>
#include <Concurrency/ThreadManager.hpp>
#include <Handlers/IMaydayHandler.hpp>
#include <KernelLog.hpp>
#include <Memory/HeapAllocator.hpp>
#include <Memory/ObjectPool.hpp>

#include "IPCPortService.hpp"

namespace Quantum::Kernel::IPC {
  IPCPortService::IPCPortService(
    ThreadManager& threads,
    IPCPortRepository& repository,
    HeapAllocator& heap,
    ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>& messagePool,
    IMaydayHandler& maydayHandler
  ) :
    _threads(threads),
    _repository(repository),
    _heap(heap),
    _messagePool(messagePool),
    _maydayHandler(maydayHandler)
  {
  }

  void IPCPortService::Send(IPCPort* port, IPCMessage* message) {
    Thread* currentThread = _threads.GetCurrent();

    currentThread->SendNode.Owner = currentThread;
    currentThread->SendNode.Next = nullptr;
    currentThread->SendNode.Previous = nullptr;

    port->SendQueue.Enqueue(&currentThread->SendNode);

    // save port ID before sleeping, port may be destroyed while we wait
    IPCPortID savedPortID = port->ID;

    // spin until we can add the message to the queue
    for (;;) {
      port->Lock.Acquire();

      // if there's room in the message queue, add the message and wake a
      // waiting receiver if there is one
      if (port->MessageQueue.GetCount() < port->MaxMessageQueueSize) {
        port->MessageQueue.Enqueue(message);

        // remove ourselves from send queue while port is still valid
        port->SendQueue.Remove(&currentThread->SendNode);

        ThreadWaitNode* receiveHead = port->ReceiveQueue.Dequeue();

        // if there is a waiting receiver, wake it up to handle the new message
        if (receiveHead) {
          Thread* threadToWake = receiveHead->Owner;

          // waking the thread may cause it to be scheduled and run before we
          // release the lock, so we need to release the lock before waking it
          // to avoid deadlock
          port->Lock.Release();

          if (threadToWake) {
            _threads.Resume(threadToWake);
          }
        } else {
          port->Lock.Release();
        }

        // message is in the queue, break regardless of whether a receiver
        // was waiting; looping would enqueue the same message again
        return;
      }

      // no room in the message queue, release the lock and sleep until a
      // receiver dequeues a message and wakes us
      port->Lock.Release();

      _threads.Sleep(
        _threads.GetCurrent(),
        0
      );

      // after waking, verify the port still exists, it may have been
      // destroyed by Unregister while we were sleeping
      if (!_repository.FindByID(savedPortID).Success) {
        // port destroyed, free the message since it was never enqueued
        if (message) {
          if (message->Payload) {
            _heap.Free(message->Payload);
          }

          _messagePool.Free(message);
        }

        // node already dequeued by Unregister
        return;
      }
    }
  }

  IPCMessage* IPCPortService::Receive(IPCPort* port) {
    Thread* currentThread = _threads.GetCurrent();

    currentThread->ReceiveNode.Owner = currentThread;
    currentThread->ReceiveNode.Next = nullptr;
    currentThread->ReceiveNode.Previous = nullptr;

    port->ReceiveQueue.Enqueue(&currentThread->ReceiveNode);

    // save port ID before sleeping, port may be destroyed while we wait
    IPCPortID savedPortID = port->ID;

    // spin until we can receive a message
    for (;;) {
      port->Lock.Acquire();

      // if there is a message in the queue, pop it and return it
      IPCMessage* message = port->MessageQueue.Dequeue();

      if (message) {
        // remove ourselves from the receive queue while port is still valid
        // under the lock; safe even if Send already dequeued us (Remove is
        // a no-op for already-removed nodes)
        port->ReceiveQueue.Remove(&currentThread->ReceiveNode);

        // dequeuing freed a slot; wake a blocked sender if one exists
        ThreadWaitNode* sendHead = port->SendQueue.GetHead();
        Thread* senderToWake = sendHead ? sendHead->Owner : nullptr;

        port->Lock.Release();

        if (senderToWake) {
          _threads.Resume(senderToWake);
        }

        return message;
      }

      port->Lock.Release();

      // sleep until a message is sent to this port, which will wake this thread
      _threads.Sleep(
        _threads.GetCurrent(),
        0
      );

      // after waking, verify the port still exists, it may have been
      // destroyed by Unregister while we were sleeping; Unregister already
      // dequeued our node from ReceiveQueue
      if (!_repository.FindByID(savedPortID).Success) {
        return nullptr;
      }
    }
  }

  Result<IPCMessage*> IPCPortService::TryReceive(IPCPort* port) {
    port->Lock.Acquire();

    IPCMessage* message = port->MessageQueue.Dequeue();

    if (message) {
      // dequeuing freed a slot; wake a blocked sender if one exists
      ThreadWaitNode* sendHead = port->SendQueue.GetHead();
      Thread* senderToWake = sendHead ? sendHead->Owner : nullptr;

      port->Lock.Release();

      if (senderToWake) {
        _threads.Resume(senderToWake);
      }

      return Result<IPCMessage*>(
        true,
        message
      );
    } else {
      port->Lock.Release();

      return Result<IPCMessage*>(false);
    }
  }

  IPCMessage* IPCPortService::ReceiveAny(
    IPCPort** ports,
    Size portCount,
    Size* outIndex
  ) {
    bool validRequest
      = ports
     && portCount > 0
     && portCount <= IPC_PORT_POOL_SIZE;

    if (validRequest) {
      Thread* current = _threads.GetCurrent();
      ThreadWaitNode nodes[IPC_PORT_POOL_SIZE] = {};
      IPCPortID savedIDs[IPC_PORT_POOL_SIZE] = {};

      for (
        Size portIndex = 0;
        portIndex < portCount;
        ++portIndex
      ) {
        nodes[portIndex].Owner = current;
        savedIDs[portIndex] = ports[portIndex]->ID;

        ports[portIndex]->ReceiveQueue.Enqueue(&nodes[portIndex]);
      }

      for (;;) {
        // sanity-check ports pointer on every iteration, catches stack
        // corruption that could zero the pointer across a Sleep call
        if (!ports) {
          KLOG_CRITICAL(
            "ports is null in ReceiveAny loop (%u ports, current is %p)",
            portCount,
            current
          );
          MAYDAY("ports became null in ReceiveAny");
        }

        // check all ports for an available message
        for (Size portIndex = 0; portIndex < portCount; ++portIndex) {
          ports[portIndex]->Lock.Acquire();

          IPCMessage* message = ports[portIndex]->MessageQueue.Dequeue();

          // capture a blocked sender to wake after releasing the lock
          Thread* senderToWake = nullptr;

          if (message) {
            ThreadWaitNode* sendHead = ports[portIndex]->SendQueue.GetHead();
            senderToWake = sendHead ? sendHead->Owner : nullptr;
          }

          ports[portIndex]->Lock.Release();

          if (message) {
            if (senderToWake) {
              _threads.Resume(senderToWake);
            }
            // unregister from all ports; re-validate each through the
            // repository since any port could have been freed between the
            // lock release above and this cleanup
            for (
              Size portIndex2 = 0;
              portIndex2 < portCount;
              ++portIndex2
            ) {
              Result<IPCPort*> findResult
                = _repository.FindByID(savedIDs[portIndex2]);

              if (findResult.Success) {
                findResult.Data->ReceiveQueue.Remove(&nodes[portIndex2]);
              }
            }

            if (outIndex) {
              *outIndex = portIndex;
            }

            return message;
          }
        }

        // no messages, sleep until any port's Send wakes us
        _threads.Sleep(
          current,
          0
        );

        // after waking, verify all ports still exist
        bool anyDestroyed = false;

        for (
          Size portIndex = 0;
          portIndex < portCount;
          ++portIndex
        ) {
          if (!_repository.FindByID(savedIDs[portIndex]).Success) {
            anyDestroyed = true;

            break;
          }
        }

        if (anyDestroyed) {
          // clean up remaining nodes from surviving ports; use the
          // repository to get a validated pointer for each
          for (
            Size portIndex = 0;
            portIndex < portCount;
            ++portIndex
          ) {
            Result<IPCPort*> findResult
              = _repository.FindByID(savedIDs[portIndex]);

            if (findResult.Success) {
              findResult.Data->ReceiveQueue.Remove(&nodes[portIndex]);
            }
          }

          return nullptr;
        }
      }
    } else {
      KLOG_WARNING(
        "Invalid arguments to ReceiveAny (ports=%p, portCount=%u)",
        ports,
        portCount
      );

      return nullptr;
    }
  }

  Result<IPCMessage*> IPCPortService::ReceiveWithTimeout(
    IPCPort* port,
    UInt64 timeoutTicks
  ) {
    Thread* currentThread = _threads.GetCurrent();

    currentThread->ReceiveNode.Owner = currentThread;
    currentThread->ReceiveNode.Next = nullptr;
    currentThread->ReceiveNode.Previous = nullptr;

    // add the current thread to the receive queue before acquiring the lock to
    // ensure proper blocking behavior if there are no messages available
    port->ReceiveQueue.Enqueue(&currentThread->ReceiveNode);

    // save port ID before sleeping, port may be destroyed while we wait
    IPCPortID savedPortID = port->ID;
    UInt64 ticksWaited = 0;

    // spin until we can receive a message or the timeout expires
    for (;;) {
      port->Lock.Acquire();

      // if there is a message in the queue, pop it and return it
      IPCMessage* message = port->MessageQueue.Dequeue();

      if (message) {
        // remove ourselves while port is still valid under the lock
        port->ReceiveQueue.Remove(&currentThread->ReceiveNode);

        // dequeuing freed a slot; wake a blocked sender if one exists
        ThreadWaitNode* sendHead = port->SendQueue.GetHead();
        Thread* senderToWake = sendHead ? sendHead->Owner : nullptr;

        port->Lock.Release();

        if (senderToWake) {
          _threads.Resume(senderToWake);
        }

        return Result<IPCMessage*>(true, message);
      } else if (ticksWaited >= timeoutTicks) {
        // timed out; remove ourselves while port is still valid
        port->ReceiveQueue.Remove(&currentThread->ReceiveNode);

        port->Lock.Release();

        return Result<IPCMessage*>(false);
      }

      port->Lock.Release();

      // sleep for a tick to avoid busy-waiting and allow other threads to
      // run, which may free up space in the message queue sooner
      _threads.Sleep(
        currentThread,
        1
      );

      ++ticksWaited;

      // after waking, verify the port still exists, it may have been
      // destroyed by Unregister while we were sleeping
      if (!_repository.FindByID(savedPortID).Success) {
        return Result<IPCMessage*>(false);
      }
    }
  }
}
