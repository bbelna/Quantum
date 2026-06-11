/**
 * @file Kernel/Memory/MemoryPressure.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryPressureMonitor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelConstants.hpp>
#include <KernelTypes.hpp>

#include "IMemoryAllocator.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Memory pressure levels based on free @ref MemoryBlock availability.
   */
  enum class MemoryPressure : UInt8 {
    /**
     * @brief Free block count is above the elevated-exit threshold; no
     *        reclaim action is needed.
     */
    Normal = 0,

    /**
     * @brief Free block count has dropped below the elevated-entry threshold.
     *        Reclaimers should begin releasing non-essential memory.
     */
    Elevated = 1,

    /**
     * @brief Free block count has dropped below the critical-entry threshold.
     *        Aggressive reclamation is required to avoid allocation failures.
     */
    Critical = 2
  };

  /**
   * @brief Snapshot of memory pressure diagnostics for reporting.
   */
  struct MemoryPressureDiagnostics {
    /**
     * @brief Total number of @ref MemoryBlock under management.
     */
    Size TotalBlocks;

    /**
     * @brief Number of @ref MemoryBlock currently free.
     */
    Size FreeBlocks;

    /**
     * @brief Number of @ref MemoryBlock currently allocated.
     */
    Size UsedBlocks;

    /**
     * @brief Total bytes mapped for the kernel heap.
     */
    Size KernelHeapBytes;

    /**
     * @brief Number of active @ref SharedBuffer in the registry.
     */
    Size SharedBufferCount;

    /**
     * @brief Total bytes reserved by the @ref IPCMessage @ref ObjectPool.
     */
    Size PoolIPCMessageBytes;

    /**
     * @brief Total bytes reserved by the @ref IPCPort @ref ObjectPool.
     */
    Size PoolIPCPortBytes;

    /**
     * @brief Total bytes reserved by the @ref SharedBuffer @ref ObjectPool.
     */
    Size PoolSharedBufferBytes;

    /**
     * @brief Current @ref MemoryPressure state at the time of the snapshot.
     */
    MemoryPressure State;
  };

  /**
   * @brief Monitors system @ref MemoryBlock pressure and dispatches reclaim
   *        callbacks when free block counts fall below configured thresholds.
   *
   * Thresholds use hysteresis to avoid rapid state transitions. All values
   * are in @ref MemoryBlock (4 KB each on IA-32).
   *
   * Typical 32 MB system: 8192 total blocks.
   *   - **Elevated entry:** free < 1024 blocks (4 MB, ~12.5%)
   *   - **Elevated exit:**  free >= 1536 blocks (6 MB, ~18.75%)
   *   - **Critical entry:** free < 256 blocks (1 MB, ~3.1%)
   *   - **Critical exit:**  free >= 512 blocks (2 MB, ~6.25%)
   */
  class MemoryPressureMonitor {
    public:
      /**
       * @brief Maximum number of registered reclaimers.
       */
      static constexpr Size MaxReclaimers = 8;

      /**
       * @brief Thresholds expressed in kernel blocks.
       * @{
       */
      static constexpr Size ElevatedEntryBlocks = 1024;
      static constexpr Size ElevatedExitBlocks = 1536;
      static constexpr Size CriticalEntryBlocks = 256;
      static constexpr Size CriticalExitBlocks = 512;
      /** @} */

      /**
       * @brief Reclaimer callback signature. Called with the target pressure
       *        level. Returns the number of @ref MemoryBlock freed.
       */
      typedef Size (*Reclaimer)(MemoryPressure level);

      /**
       * @brief Constructs the monitor with references to kernel subsystems
       *        used to query memory statistics.
       * @param allocator The kernel memory allocator.
       * @param heap The kernel heap allocator.
       * @param sharedBuffers The shared buffer registry.
       * @param ipcMessagePool The IPC message object pool.
       * @param ipcPortPool The IPC port object pool.
       * @param sharedBufferPool The shared buffer object pool.
       */
      explicit MemoryPressureMonitor(
        IMemoryAllocator& allocator,
        HeapAllocator* heap,
        SharedBufferRepository* sharedBuffers,
        ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>* ipcMessagePool,
        ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>* ipcPortPool,
        ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>* sharedBufferPool
      );

      /**
       * @brief Registers a reclaim callback. Callbacks are invoked in
       *        registration order during reclaim.
       * @param func The reclaim function to register.
       * @return `true` if registered successfully; `false` if the table
       *         is full.
       */
      bool RegisterReclaimer(Reclaimer func);

      /**
       * @brief Recomputes the pressure state from current free block counts
       *        with hysteresis.
       * @return The updated pressure state.
       */
      MemoryPressure UpdateState();

      /**
       * @brief Invokes registered reclaimers in order, targeting the given
       *        pressure level.
       * @note Safe to call repeatedly (idempotent).
       * @param targetLevel The pressure level to reclaim toward. Reclaimers
       *                    receive this value so they can adjust
       *                    aggressiveness.
       * @return Total @ref MemoryBlock freed across all reclaimers.
       */
      Size Reclaim(MemoryPressure targetLevel);

      /**
       * @brief Returns the current @ref MemoryPressure state without
       *        recomputing.
       * @return The cached @ref MemoryPressure state.
       */
      MemoryPressure GetState() const {
        return _state;
      }

      /**
       * @brief Returns `true` if pressure is elevated or critical.
       * @return `true` when the system is under memory pressure.
       */
      bool IsElevated() const {
        return _state >= MemoryPressure::Elevated;
      }

      /**
       * @brief Returns `true` if pressure is critical.
       * @return `true` when the system is critically low on free
       *         @ref MemoryBlock.
       */
      bool IsCritical() const {
        return _state == MemoryPressure::Critical;
      }

      /**
       * @brief Fills a diagnostics snapshot with current memory statistics.
       * @param out Pointer to the @ref MemoryPressureDiagnostics to fill.
       */
      void GetDiagnostics(MemoryPressureDiagnostics* out) const;

    private:
      /**
       * @brief Reference to a concrete implementation of @ref IMemoryAllocator.
       */
      IMemoryAllocator& _memoryAllocator;

      /**
       * @brief Reference to the @ref HeapAllocator.
       */
      HeapAllocator* _heapAllocator = nullptr;

      /**
       * @brief Reference to the @ref SharedBufferRepository.
       */
      SharedBufferRepository* _sharedBuffers = nullptr;

      /**
       * @brief Reference to the @ref IPCMessage @ref ObjectPool.
       */
      ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>* _ipcMessagePool = nullptr;

      /**
       * @brief Reference to the @ref IPCPort @ref ObjectPool.
       */
      ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>* _ipcPortPool = nullptr;

      /**
       * @brief Reference to the @ref SharedBuffer @ref ObjectPool.
       */
      ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>* _sharedBufferPool
        = nullptr;

      /**
       * @brief Current @ref MemoryPressure state.
       */
      MemoryPressure _state = MemoryPressure::Normal;

      /**
       * @brief Registered reclaimer callbacks.
       */
      Reclaimer _reclaimers[MaxReclaimers] = {};

      /**
       * @brief Number of registered reclaimers.
       */
      Size _reclaimerCount = 0;
  };
}
