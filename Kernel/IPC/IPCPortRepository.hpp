/**
 * @file Kernel/IPC/IPCPortRepository.hpp
 * @brief Declares @ref @QKrnl::IPC::IPCPortRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelConstants.hpp>

#include "IPCPort.hpp"

namespace Quantum::Kernel::IPC {
  /**
   * @brief Repository for @ref IPCPort.
   */
  class IPCPortRepository {
    public:
      /**
       * @brief Creates a new @ref IPCPortRepository.
       * @param heap Pointer to the @ref @QKrnl @ref HeapAllocator.
       * @param messagePool Pointer to the @ref @QKrnl @ref IPCMessage
       *                     @ref ObjectPool.
       * @param portPool Pointer to the @ref @QKrnl @ref IPCPort
       *                 @ref ObjectPool.
       * @param threads Pointer to the @ref @QKrnl @ref ThreadManager.
       */
      IPCPortRepository(
        HeapAllocator& heap,
        ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>& messagePool,
        ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>& portPool,
        ThreadManager& threads
      );

      /**
       * @brief Saves a new @ref IPCPort to the repository.
       * @param port Pointer to the @ref IPCPort to register.
       * @return
       *   @ref Result indicating success or failure of the registration
       *   operation (see @ref Result @ref Result::Success).
       */
      Result<> Save(IPCPort* port);

      /**
       * @brief Removes an @ref IPCPort from the repository by its
       *        @ref IPCPortID.
       * @note Caller is responsible for freeing the removed @ref IPCPort.
       * @param id @ref IPCPortID of the @ref IPCPort to remove.
       * @return
       *   @ref Result indicating success or failure (see @ref Result
       *   @ref Result::Success).
       */
      Result<> RemoveByID(IPCPortID id);

      /**
       * @brief Finds a @ref IPCPort by its @ref IPCPortID.
       * @param id @ref IPCPortID of the @ref IPCPort to find.
       * @return
       *   @ref If found, a @ref Result with @ref Result::Success as
       *   `true` and @ref Result::Data holding a pointer to the found
       *   @ref IPCPort. If not found, a @ref Result with
       *   @ref Result::Success as `false` and @ref Result::Data as `nullptr`.
       */
      Result<IPCPort*> FindByID(IPCPortID id);

      /**
       * @brief Allocates the next available auto-assigned @ref IPCPortID.
       * @return The next available auto-assigned @ref IPCPortID, or an invalid
       *         @ref IPCPortID if none are available.
       */
      IPCPortID AllocateID();

      /**
       * @brief Blocks the calling thread until the specified port exists.
       *
       * If the port is already registered, returns immediately. Otherwise
       * the thread is put to sleep and woken when @ref Save registers a
       * port with the matching ID.
       *
       * @param portID The @ref IPCPortID to wait for.
       * @return `true` if the port now exists; `false` on failure (e.g.,
       *         waiter table full).
       */
      bool WaitForPort(IPCPortID portID);

    private:
      /**
       * @brief Maximum number of concurrent port waiters.
       */
      static constexpr Size MaxPortWaiters = 32;
      /**
       * @brief Reference to the @ref @QKrnl @ref HeapAllocator.
       */
      HeapAllocator& _heap;

      /**
       * @brief Reference to the @ref @QKrnl @ref IPCMessage @ref ObjectPool.
       */
      ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>& _messagePool;

      /**
       * @brief Reference to the @ref @QKrnl @ref IPCPort @ref ObjectPool.
       */
      ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>& _portPool;

      /**
       * @brief Reference to the @ref @QKrnl @ref ThreadManager.
       */
      ThreadManager& _threads;

      /**
       * @brief @ref Spinlock protecting repository operations.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief @ref IDAllocator for auto-assigned port IDs (@ref IPCPortID).
       */
      IDAllocator<IPCPortID, IPC_MAX_AUTO_ASSIGN_PORTS> _portIDs;

      /**
       * @brief Persistence list for the repository's @ref IPCPort instances.
       */
      LinkedList<IPCPort*> _ports;

      /**
       * @brief Port IDs that threads are waiting for.
       */
      IPCPortID _waiterPortIDs[MaxPortWaiters] = {};

      /**
       * @brief Threads waiting for the corresponding port ID.
       */
      Thread* _waiterThreads[MaxPortWaiters] = {};

      /**
       * @brief Number of active port waiters.
       */
      Size _waiterCount = 0;
  };
}
