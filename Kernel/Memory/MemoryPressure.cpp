/**
 * @file Kernel/Memory/MemoryPressure.cpp
 * @brief Implements @ref @QKrnl::Memory::MemoryPressureMonitor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <IPC/IPCPort.hpp>

#include "HeapAllocator.hpp"
#include "MemoryPressure.hpp"
#include "ObjectPool.hpp"
#include "SharedBufferRepository.hpp"

namespace Quantum::Kernel::Memory {
  MemoryPressureMonitor::MemoryPressureMonitor(
    IMemoryAllocator& memoryAllocator,
    HeapAllocator* heapAllocator,
    SharedBufferRepository* sharedBuffers,
    ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>* ipcMessagePool,
    ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>* ipcPortPool,
    ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>* sharedBufferPool
  ) :
    _memoryAllocator(memoryAllocator),
    _heapAllocator(heapAllocator),
    _sharedBuffers(sharedBuffers),
    _ipcMessagePool(ipcMessagePool),
    _ipcPortPool(ipcPortPool),
    _sharedBufferPool(sharedBufferPool)
  {
  }

  bool MemoryPressureMonitor::RegisterReclaimer(Reclaimer reclaimer) {
    if (
      reclaimer &&
      _reclaimerCount < MaxReclaimers
    ) {
      _reclaimers[_reclaimerCount++] = reclaimer;

      return true;
    } else {
      return false;
    }
  }

  MemoryPressure MemoryPressureMonitor::UpdateState() {
    Size total = _memoryAllocator.GetBlockCount();
    Size used = _memoryAllocator.GetUsedBlockCount();
    Size free
      = total > used
      ? (total - used)
      : 0;

    switch (_state) {
      case MemoryPressure::Normal: {
        if (free < CriticalEntryBlocks) {
          _state = MemoryPressure::Critical;
        } else if (free < ElevatedEntryBlocks) {
          _state = MemoryPressure::Elevated;
        }

        break;
      }

      case MemoryPressure::Elevated: {
        if (free < CriticalEntryBlocks) {
          _state = MemoryPressure::Critical;
        } else if (free >= ElevatedExitBlocks) {
          _state = MemoryPressure::Normal;
        }

        break;
      }

      case MemoryPressure::Critical: {
        if (free >= CriticalExitBlocks) {
          if (free >= ElevatedExitBlocks) {
            _state = MemoryPressure::Normal;
          } else {
            _state = MemoryPressure::Elevated;
          }
        }

        break;
      }
    }

    return _state;
  }

  Size MemoryPressureMonitor::Reclaim(MemoryPressure targetLevel) {
    Size totalFreed = 0;

    for (
      Size reclaimerIndex = 0;
      reclaimerIndex < _reclaimerCount;
      ++reclaimerIndex
    ) {
      if (_reclaimers[reclaimerIndex]) {
        totalFreed += _reclaimers[reclaimerIndex](targetLevel);
      }
    }

    UpdateState();

    return totalFreed;
  }

  void MemoryPressureMonitor::GetDiagnostics(
    MemoryPressureDiagnostics* out
  ) const {
    if (out) {
      out->TotalBlocks = _memoryAllocator.GetBlockCount();
      out->UsedBlocks = _memoryAllocator.GetUsedBlockCount();
      out->FreeBlocks
        = out->TotalBlocks > out->UsedBlocks
        ? (out->TotalBlocks - out->UsedBlocks)
        : 0;
      out->KernelHeapBytes
        = _heapAllocator
        ? _heapAllocator->GetMappedBytes()
        : 0;
      out->State = _state;
      out->SharedBufferCount = 0;

      if (_sharedBuffers) {
        out->SharedBufferCount = _sharedBuffers->GetCount();
      }

      out->PoolIPCMessageBytes
        = _ipcMessagePool
        ? _ipcMessagePool->GetTotalBytes()
        : 0;
      out->PoolIPCPortBytes
        = _ipcPortPool
        ? _ipcPortPool->GetTotalBytes()
        : 0;
      out->PoolSharedBufferBytes
        = _sharedBufferPool
        ? _sharedBufferPool->GetTotalBytes()
        : 0;
    }
  }
}
