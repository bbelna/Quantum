/**
 * @file Kernel/IPC/IPCPortService.hpp
 * @brief Declares @ref @QKrnl::IPC::IPCPortService.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IPCPort.hpp"
#include "IPCPortRepository.hpp"

namespace Quantum::Kernel::IPC {
  /**
   * @brief Performs operations on @ref IPCPort instances.
   *
   * Provides send/receive functionality that operates on @ref IPCPort
   * instances. When a send or receive cannot be satisfied immediately
   * (queue full or empty), the calling @ref Thread is blocked via the
   * @ref ThreadManager and enqueued on the port's send or receive wait queue
   * until the operation can complete.
   *
   * All public methods are called from ABI / system call handlers with
   * interrupts disabled and the port's spinlock held by the caller.
   */
  class IPCPortService {
    public:
      /**
       * @brief Creates a new @ref IPCPortService.
       * @param threads Pointer to the @ref @QKrnl @ref ThreadManager.
       * @param registry Pointer to the @ref @QKrnl @ref IPCPortRepository.
       * @param heap Pointer to the @ref @QKrnl @ref HeapAllocator.
       * @param messagePool Pointer to the @ref @QKrnl @ref IPCMessage
       *                    @ref ObjectPool.
       * @param maydayHandler Pointer to the @ref @QKrnl
       *                      @ref IMaydayHandler.
       */
      IPCPortService(
        ThreadManager& threads,
        IPCPortRepository& registry,
        HeapAllocator& heap,
        ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>& messagePool,
        IMaydayHandler& maydayHandler
      );

      /**
       * @brief Sends an @ref IPCMessage to the specified @ref IPCPort.
       * @note If the @ref IPCMessage queue is full, the calling @ref Thread
       *       will be blocked until space is available.
       * @param port Pointer to the @ref IPCPort to send the message to.
       * @param message Pointer to the @ref IPCMessage to send.
       */
      void Send(IPCPort* port, IPCMessage* message);

      /**
       * @brief Receives an @ref IPCMessage from the specified @ref IPCPort.
       * @note If there are no @ref IPCMessage available, the calling
       *       @ref Thread will be blocked until an @ref IPCMessage arrives.
       * @param port Pointer to the @ref IPCPort to receive a message from.
       * @return Pointer to the received @ref IPCMessage.
       */
      IPCMessage* Receive(IPCPort* port);

      /**
       * @brief Tries to receive an @ref IPCMessage from the specified
       *        @ref IPCPort without blocking.
       * @note If there are no @ref IPCMessage available, an error will be
       *       returned instead.
       * @param port Pointer to the @ref IPCPort to receive a message from.
       * @return @ref Result containing a pointer to the received
       *         @ref IPCMessage if successful, or an error if there were no
       *         @ref IPCMessage available.
       */
      Result<IPCMessage*> TryReceive(IPCPort* port);

      /**
       * @brief Receives an @ref IPCMessage from the specified @ref IPCPort
       *        within the specified @ref timeoutTicks.
       * @note If there are no @ref IPCMessage available, the calling
       *       @ref Thread will be blocked until an @ref IPCMessage arrives or
       *       the timeout expires.
       * @param port Pointer to the @ref IPCPort to receive an @ref IPCMessage
       *             from.
       * @param timeoutTicks Number of ticks to wait before timing out.
       * @return @ref Result containing the received @ref IPCMessage if
       *         successful, or an error if the timeout expired or the
       *         @ref IPCPort was destroyed while waiting.
       */
      Result<IPCMessage*> ReceiveWithTimeout(
        IPCPort* port,
        UInt64 timeoutTicks
      );

      /**
       * @brief Blocks until an @ref IPCMessage arrives on any of the specified
       *        @ref IPCPort instances.
       * @param ports Array of @ref IPCPort pointers to wait on.
       * @param portCount Number of @ref IPCPort pointers in the array
       *                  (max `16`).
       * @param outIndex Receives the index of the @ref IPCPort that had the
       *                 @ref IPCMessage.
       * @return Pointer to the received @ref IPCMessage, or `nullptr` if all
       *         ports were destroyed.
       */
      IPCMessage* ReceiveAny(
        IPCPort** ports,
        Size portCount,
        Size* outIndex
      );

    private:
      /**
       * @brief Reference to the @ref @QKrnl @ref ThreadManager.
       */
      ThreadManager& _threads;

      /**
       * @brief Reference to the @ref @QKrnl @ref IPCPortRepository.
       */
      IPCPortRepository& _repository;

      /**
       * @brief Reference to the @ref @QKrnl @ref HeapAllocator.
       */
      HeapAllocator& _heap;

      /**
       * @brief Reference to the @ref @QKrnl @ref IPCMessage @ref ObjectPool.
       */
      ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>& _messagePool;

      /**
       * @brief Reference to @ref @QKrnl @ref IMaydayHandler.
       */
      IMaydayHandler& _maydayHandler;
  };
}
