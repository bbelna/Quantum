/**
 * @file Kernel/Kernel.cpp
 * @brief Implements @ref @QKrnl::Kernel.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Version.hpp>

#include "Concurrency/ProcessManager.hpp"
#include "Concurrency/StartThreading.hpp"
#include "Concurrency/ThreadManager.hpp"
#include "Drivers/CPU/ICPUDriver.hpp"
#include "Drivers/Timers/ITimerDriver.hpp"
#include "Interrupts/IInterruptManager.hpp"
#include "Kernel.hpp"
#include "KernelLog.hpp"
#include "Memory/AddressSpaceMap.hpp"
#include "Memory/IAddressTranslator.hpp"
#include "Memory/IMemoryAllocator.hpp"
#include "Memory/IMemoryMapper.hpp"
#include "Concurrency/ConcurrencyModule.hpp"
#include "Handlers/HandlersModule.hpp"
#include "IPC/IPCModule.hpp"
#include "Memory/MemoryModule.hpp"
#include "Resources/KernelResourcesModule.hpp"

namespace Quantum::Kernel {
  void Kernel::Initialize(
    IKernelInitializer* initializer,
    InitialProcessInfo initialProcess
  ) {
    Context = &_context;

    initializer->Initialize(&_context);

    if (!_didInitializeSuccessfully()) {
      MAYDAY(
        "Kernel initialization failed; halting. "
        "Check previous log messages for details."
      );
    }

    _registerModules();

    if (!_modules.InitializeAll(&_context)) {
      MAYDAY(
        "Module initialization failed; halting. "
        "Check previous log messages for details."
      );
    }

    _initializeInitialProcess(initialProcess);

    _logVersion();
    _startThreading();
  }

  void Kernel::_registerModules() {
    static Memory::MemoryModule memoryModule;
    static Resources::KernelResourcesModule resourcesModule;
    static Concurrency::ConcurrencyModule concurrencyModule;
    static IPC::IPCModule ipcModule;
    static Handlers::HandlersModule handlersModule;

    _modules.Register(&memoryModule);
    _modules.Register(&resourcesModule);
    _modules.Register(&concurrencyModule);
    _modules.Register(&ipcModule);
    _modules.Register(&handlersModule);
  }

  IInterruptContext* Kernel::Tick(IInterruptContext& context) {
    if (_context.Timer) {
      context = *_context.Timer->Tick(context);
    }

    return Yield(context);
  }

  IInterruptContext* Kernel::Yield(IInterruptContext& context) {
    if (_context.ThreadManager) {
      IInterruptContext* result = _context.ThreadManager->Tick(&context);

      if (_context.ProcessesManager) {
        _context.ProcessesManager->ReapZombies();
      }

      return result;
    } else {
      return &context;
    }
  }

  bool Kernel::_didInitializeSuccessfully() {
    // check if all critical platform-specific components were initialized
    // TODO: actually implement this lol
    return _context.Log
       &&  _context.Timer;
  }

  void Kernel::_logVersion() {
    _context.Log->Info(
      "Quantum " QUANTUMOS_RELEASE " " PLATFORM "/" TARGET_ARCH " "
      __DATE__ " " __TIME__
    );
  }

  void Kernel::_initializeInitialProcess(InitialProcessInfo initialProcess) {
    KLOG_TRACE(
      "Initializing initial process \"%s\"",
      initialProcess.Name
    );

    _mapInitialImage(initialProcess.ImageBlock);

    UIntPtr imageBaseAddress = _context.InitialProcessAddress;

    _initializeInitialProcessThread(
      initialProcess,
      imageBaseAddress
    );
  }

  void Kernel::_initializeInitialProcessThread(
    InitialProcessInfo initialProcess,
    UIntPtr imageBaseAddress
  ) {
    UIntPtr entryPoint
      = imageBaseAddress
      + initialProcess.EntryPointOffset;
    Size blockSize = _context.MemoryAllocator->GetBlockSize();
    UIntPtr imageDataAddress
      = imageBaseAddress
      + AlignUp<Size>(
          initialProcess.ProcessImageSize,
          blockSize
        );

    KLOG_TRACE(
      "Initializing initial process \"%s\" "
      "(EntryPoint %p, ImageDataAddress %p)",
      initialProcess.Name,
      entryPoint,
      imageDataAddress
    );

    // pack the image data address as binary argument data; the initial
    // process reads it back via *reinterpret_cast<UIntPtr*>(argv[0])
    const char* argumentData = reinterpret_cast<const char*>(&imageDataAddress);
    Size argumentDataSize = sizeof(imageDataAddress);
    static Process* process = _context.ProcessesManager->Spawn(
      initialProcess.Name,
      entryPoint,
      _context.ProcessesManager->GetKernelProcess(),
      1,
      argumentData,
      argumentDataSize
    );

    // register the full initial image (binary + bundle data) in the
    // process's address space map so that syscall pointer validation
    // accepts addresses within the mapped region
    Size imageBlockSize
      = AlignUp<Size>(
        initialProcess.ImageBlock.SizeInBytes,
        blockSize
      );

    process->AddressSpaceMap
      .Insert(
        MemoryMapping {
          process->AddressSpace,
          MemoryBlock {
            imageBaseAddress,
            imageBlockSize
          },
          MemoryBlock {
            _context.AddressTranslator
              ->VirtualToPhysical(initialProcess.ImageBlock.Base),
            imageBlockSize
          },
          MemoryMappingFlags {
            MemoryMappingPermissions::Read
              | MemoryMappingPermissions::Write
              | MemoryMappingPermissions::User,
            MemoryMappingCache::Default,
            MemoryMappingOptions::None
          },
          MemoryRegionType::Shared
        }
      );

    _context.ThreadManager->Start(process->MainThread);
  }

  void Kernel::_mapInitialImage(MemoryBlock imageBlock) {
    KLOG_TRACE(
      "Mapping initial image (%p, %u bytes)",
      imageBlock.Base,
      imageBlock.SizeInBytes
    );

    MemoryBlock kernelBlock {
      _context.AddressTranslator->VirtualToPhysical(imageBlock.Base),
      imageBlock.SizeInBytes
    };
    Size blockSize = _context.MemoryAllocator->GetBlockSize();
    Size blockCount
      = AlignUp<Size>(
          kernelBlock.SizeInBytes,
          blockSize
        )
      / blockSize;

    // map the initial image into the kernel address space
    // fyi: mapping into the kernel's address space "legitimizes" the mapping;
    // until a mapping exists in the kernel's address space, it doesn't exist
    // at all; in this way the kernel address space serves as "physical" memory
    // backing all allocations
    for (
      Size blockIndex = 0;
      blockIndex < blockCount;
      blockIndex++
    ) {
      MemoryBlock fromBlock {
        _context.InitialProcessAddress
          + blockIndex * blockSize,
        blockSize
      };
      MemoryBlock toBlock {
        kernelBlock.Base
          + blockIndex * blockSize,
        blockSize
      };
      const MemoryMappingFlags flags {
        MemoryMappingPermissions::Read
          | MemoryMappingPermissions::Write
          | MemoryMappingPermissions::User,
        MemoryMappingCache::Default,
        MemoryMappingOptions::None
      };

      // ensure the mapping succeeds, if it doesn't, we have to mayday
      if (
        _context.MemoryMapper
          ->Map(
              *_context.KernelAddressSpace,
              fromBlock,
              toBlock,
              flags
            )
           .SizeInBytes > 0
      ) {
        // record the region in the kernel address space map for bookkeeping
        _context.KernelAddressSpaceMap
          ->Insert(
              MemoryRegion {
                toBlock,
                flags,
                MemoryRegionType::Shared
              }
            );
      } else {
        MAYDAY("Failed to map initial image");
      }
    }
  }

  void Kernel::_startThreading() {
    Thread* idleThread
      = _context.ThreadManager
          ->GetIdleThread();

    if (!idleThread) {
      MAYDAY("Idle thread is null");
    } else if (!idleThread->Context) {
      MAYDAY("Idle thread has no context");
    } else {
      KLOG_TRACE(
        "Bootstrapping to TID %u at IP %p",
        idleThread->ID,
        _context.ThreadContextManager
          ->GetInstructionPointer(idleThread->Context)
      );

      _context.CPU
        ->DisableInterrupts();
      _context.ThreadManager
        ->SetCurrent(idleThread);

      // TODO: refactor this into manager
      StartThreading(idleThread->Context);
    }
  }
}
