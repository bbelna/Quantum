/**
 * @file Kernel/OutOfMemoryHandler.cpp
 * @brief Implements @ref @QKrnl::OutOfMemoryHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/ProcessManager.hpp>
#include <IPC/IPCPort.hpp>
#include <KernelLog.hpp>
#include <Memory/ObjectPool.hpp>
#include <Memory/SharedBufferRepository.hpp>

#include "OutOfMemoryHandler.hpp"

namespace Quantum::Kernel::Handlers {
  OutOfMemoryHandler::OutOfMemoryHandler(
    KernelContext* context
  ) : _context(context) {}

  bool OutOfMemoryHandler::Handle() {
    _dumpUserMemoryUsage();
    _dumpKernelMemoryUsage();

    // no reclamation yet
    return false;
  }

  void OutOfMemoryHandler::_dumpUserMemoryUsage() {
    Log* log = _context->Log;

    log->Critical(">>> USER MEMORY DUMP <<<");

    // dump per-process page counts
    if (_context->ProcessesManager) {
      for (
        Size processIndex = 0;
        processIndex < PROCESS_MAX_COUNT;
        ++processIndex
      ) {
        Process* process = _context->ProcessesManager->GetByID(processIndex);

        if (!process || process->State == ProcessState::Zombie) continue;

        log->Critical(
          "PID %u (%s) has %u blocks (%u KB), heap is %u blocks (%u KB)",
          process->ID,
          process->Name,
          process->TotalBlockCount,
          process->TotalBlockCount * 4,
          process->HeapBlockCount,
          process->HeapBlockCount * 4
        );
      }
    }

    log->Critical(">>> END USER MEMORY DUMP <<<");

  }

  void OutOfMemoryHandler::_dumpKernelMemoryUsage() {
    Log* log = _context->Log;

    log->Critical(">>> KERNEL MEMORY DUMP <<<");

    // dump shared buffer count
    if (_context->SharedBufferRepository) {
      log->Critical(
        "%u shared buffers active",
        _context->SharedBufferRepository->GetCount()
      );
    }

    // dump per-tag block allocation summary
    if (_context->MemoryAllocator) {
      for (
        UInt8 tagIndex = 0;
        tagIndex < static_cast<UInt8>(MemoryBlockTag::Count);
        ++tagIndex
      ) {
        MemoryBlockTag tag = static_cast<MemoryBlockTag>(tagIndex);
        Size count = _context->MemoryAllocator->GetUsedBlockCountByTag(tag);

        if (count == 0) continue;

        log->Critical(
          "  %s %u blocks (%uKB)",
          MemoryBlockTagName(tag),
          count,
          count * 4
        );
      }
    }

    // dump pool stats
    if (_context->IPCMessagePool) {
      log->Critical(
        "IPC message pool: %u in use, %u total bytes",
        _context->IPCMessagePool->GetInUseCount(),
        _context->IPCMessagePool->GetTotalBytes()
      );
    }

    if (_context->IPCPortPool) {
      log->Critical(
        "IPC port pool: %u in use, %u total bytes",
        _context->IPCPortPool->GetInUseCount(),
        _context->IPCPortPool->GetTotalBytes()
      );
    }

    log->Critical(">>> END KERNEL MEMORY DUMP <<<");
  }
}
