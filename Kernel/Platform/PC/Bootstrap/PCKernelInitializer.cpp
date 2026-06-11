/**
 * @file Kernel/Platform/PC/Bootstrap/PCKernelInitializer.cpp
 * @brief Implements @ref @QKrnlPC::Bootstrap::PCKernelInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>
#include <Memory/AddressSpaceMap.hpp>
#include <Memory/HeapAllocator.hpp>
#include <Memory/MemoryMappingFlags.hpp>
#include <Platform/PC/Drivers/Bus/ChipsetDriver.hpp>
#include <Platform/PC/Drivers/Bus/PCI.hpp>
#include <Platform/PC/Drivers/DMA/Intel8237ADriver.hpp>
#include <Platform/PC/Drivers/Graphics/S3ViRGEDriver.hpp>
#include <Platform/PC/Drivers/Graphics/VESADriver.hpp>
#include <Platform/PC/Drivers/Interrupts/Intel8259Driver.hpp>
#include <Platform/PC/Drivers/RTC/MC146818Driver.hpp>
#include <Platform/PC/Drivers/Serial/SerialCOMDriver.hpp>
#include <Platform/PC/Drivers/Timers/Intel8253Driver.hpp>

#include "PCKernelInitializer.hpp"

namespace Quantum::Kernel::Platform::PC::Bootstrap {
  void PCKernelInitializer::Initialize(KernelContext* context) {
    _initializeLog(context);
    _initializeCPUDriver(context);
    _initializePCI(context);
    _initializeHandlers(context);
    _initializeMemory(context);
    _initializeScreen(context);
    _initializeInterrupts(context);
    _initializeThreading(context);
    _initializeUserMode(context);
    _initializeDevices(context);
  }

  void PCKernelInitializer::_initializePCI(KernelContext* context) {
    static PCI pci(*context->CPU);

    _pci = &pci;
  }

  void PCKernelInitializer::_initializeLog(KernelContext* context) {
    static SerialCOMDriver serial(SerialCOMDriver::Port::COM1);
    static VESADriver vesa;

    _serial = &serial;
    _vesa = &vesa;

    static ScreenLogSink screenSink(context->LogLevel);

    // screen sink starts pointing at VESA; redirected to S3 ViRGE (if present)
    // in _initializeScreen once the framebuffer is mapped
    screenSink.Redirect(&vesa);

    _screenSink = &screenSink;

    static LogSink serialLogger(
      [](char character) {
        serial.Write(character);
      },
      context->LogLevel
    );

    // screen logger gets raised later once app server starts; so we lock
    // the serial logger to ensure it continues logging even after the app
    // server starts
    serialLogger.SetMinLevel(SerialCOMLogLevel);
    serialLogger.LockMinLevel();

    static LogSink* loggers[2] = { &serialLogger, &screenSink };
    static SpinlockLog log(loggers, 2);

    context->Log = &log;
  }

  void PCKernelInitializer::_initializeCPUDriver(KernelContext* context) {
    static IA32CPUDriver cpu;

    context->CPU = &cpu;
  }

  void PCKernelInitializer::_initializeHandlers(KernelContext* context) {
    static IA32MaydayHandler maydayHandler(context);

    context->MaydayHandler = &maydayHandler;
  }

  void PCKernelInitializer::_initializeMemory(KernelContext* context) {
    static IA32MemoryAllocator memoryAllocator;
    static IA32PageDirectoryManager pageDirectoryManager;
    static IA32MemoryMapper memoryMapper;
    static IA32AddressTranslator addressTranslator;
    static KernelAddressSpaceMap addressSpaceMap;

    memoryAllocator.Initialize(
      context->MaydayHandler,
      BootInfo
    );

    context->MemoryAllocator = &memoryAllocator;

    auto* cpu = static_cast<IA32CPUDriver*>(context->CPU);

    pageDirectoryManager.Initialize(cpu, &memoryAllocator);

    memoryMapper.Initialize(
      cpu,
      &memoryAllocator,
      &pageDirectoryManager
    );

    addressTranslator.Initialize(cpu);

    context->MemoryMapper = &memoryMapper;
    context->AddressTranslator = &addressTranslator;
    context->AddressSpaceAllocator = &pageDirectoryManager;
    context->KernelBase = KERNEL_HIGHER_HALF_VIRTUAL_BASE;
    context->InitialProcessAddress = BootInfo->InitialProcessAddress;
    context->KernelAddressSpace = pageDirectoryManager.GetKernelPageDirectory();

    context->Log->Debug(
      "KernelAddressSpace=%p",
      reinterpret_cast<UIntPtr>(context->KernelAddressSpace)
    );

    cpu->LoadPageDirectory(
      cpu->VirtualToPhysical(
        reinterpret_cast<UIntPtr>(context->KernelAddressSpace)
      )
    );
    cpu->EnablePaging();
    cpu->InvalidatePage(0);
    cpu->NotifyKernelPageDirectoryLoaded();

    // enable top-down allocation so that the heap grows downwards from the
    // high address of the process address space, preserving low memory for
    // DMA and other hardware requirements; this must be done after paging is
    // enabled so that the allocator can access all of kernel memory through
    // the identity mapping, but before the heap allocator is initialized
    // since it relies on top-down allocation to place the heap at the high
    // end of the process address space
    memoryAllocator.EnableTopDownAllocation();

    static HeapAllocator heapAllocator;

    heapAllocator.Initialize(
      &memoryAllocator,
      &memoryMapper,
      context->MaydayHandler,
      context->MemoryPressureMonitor,
      HeapAllocatorConfiguration {
        .HeapAddressSpace = context->KernelAddressSpace,
        .HeapBlock = {
          HEAP_BASE,
          HEAP_SIZE_IN_BYTES
        },
        .GuardBlocksBefore = HEAP_GUARD_PAGE_COUNT_BEFORE,
        .GuardBlocksAfter = HEAP_GUARD_PAGE_COUNT_AFTER,
        .AlignedMagic = HEAP_ALIGNED_MAGIC,
        .AllocatedPoison = HEAP_POISON_ALLOCATED,
        .FreedPoison = HEAP_POISON_FREED,
        .Canary = HEAP_CANARY_VALUE,
        .AllocatedSentinel = HEAP_ALLOCATED_SENTINEL,
        .RequiredTailBlocks = HEAP_REQUIRED_TAIL_PAGE_COUNT
      }
    );

    context->HeapAllocator = &heapAllocator;

    // null guard: reserve the first page so allocations never return 0
    addressSpaceMap.Insert(
      MemoryRegion {
        MemoryBlock {
          0,
          memoryAllocator.GetBlockSize()
        },
        MemoryMappingFlags {
          MemoryMappingPermissions::None,
          MemoryMappingCache::Default,
          MemoryMappingOptions::Guard
        },
        MemoryRegionType::Guard
      }
    );

    // mark the entire kernel area as occupied in the kernel address space map
    // so that no allocations are made there
    addressSpaceMap.Insert(
      MemoryRegion {
        MemoryBlock {
          context->KernelBase,
          MaxAddress - context->KernelBase
        },
        MemoryMappingFlags {
          MemoryMappingPermissions::Read
            | MemoryMappingPermissions::Write
            | MemoryMappingPermissions::Execute,
          MemoryMappingCache::Default,
          MemoryMappingOptions::None
        },
        MemoryRegionType::Guard
      }
    );

    context->KernelAddressSpaceMap = &addressSpaceMap;

    static_cast<IA32CPUDriver*>(context->CPU)->InitializePAT();

    static IA32StackManagerConfigurationProvider stackConfigProvider;

    context->StackManagerConfigurationProvider = &stackConfigProvider;
  }

  void PCKernelInitializer::_initializeScreen(KernelContext* context) {
    _vesa->SetDependencies(
      context->MaydayHandler,
      context->MemoryAllocator,
      context->SharedBufferRepository
    );
    _vesa->MapFramebuffer(context->MemoryMapper, context->KernelAddressSpace);

    // TODO: dynamic device detection
    if (S3ViRGEDriver::Discover(_pci)) {
      static S3ViRGEDriver s3ViRGEDriver(
        _pci,
        context->CPU,
        context->MemoryAllocator,
        context->SharedBufferRepository
      );

      if (
        s3ViRGEDriver.Initialize(
          context->MemoryMapper,
          context->KernelAddressSpace
        )
      ) {
        _s3Virge = &s3ViRGEDriver;

        // sync text cursor from VESA so S3 ViRGE continues where boot
        // log left off instead of overwriting from (0, 0)
        s3ViRGEDriver.SetTextCursorPosition(_vesa->GetTextCursorPosition());

        // redirect kernel logger output to S3 ViRGE
        _screenSink->Redirect(&s3ViRGEDriver);
      }
    }
  }

  void PCKernelInitializer::_initializeInterrupts(KernelContext* context) {
    static Intel8259Driver pic(
      *context->CPU,
      IRQ_BASE_VECTOR,
      IRQ_BASE_VECTOR + 8
    );
    static IA32InterruptManager interruptManager(pic, context);
    static Intel8253Driver pit(Intel8253Driver::DefaultFrequency);

    // register timer interrupt handler and unmask the timer IRQ
    interruptManager.SetHandler(
      IRQ_VECTOR_TIMER,
      [](IInterruptContext& ctx) -> IInterruptContext* {
        return Kernel::Instance().Tick(ctx);
      }
    );
    interruptManager.SetHandler(
      IRQ_VECTOR_YIELD,
      [](IInterruptContext& ctx) -> IInterruptContext* {
        return Kernel::Instance().Yield(ctx);
      }
    );
    pic.Unmask(IRQ_TIMER);

    context->InterruptController = &pic;
    context->InterruptManager = &interruptManager;
    context->Timer = &pit;

    context->CPU->EnableInterrupts();
  }

  void PCKernelInitializer::_initializeThreading(KernelContext* context) {
    constexpr UInt32 sysenterCS = 0x174;
    constexpr UInt32 sysenterESP = 0x175;
    constexpr UInt32 sysenterEIP = 0x176;

    UInt32 currentESP;

    // initialize the TSS with a temporary kernel stack
    // the actual per-thread stack will be set during context switches
    // for now, use the current stack pointer as a placeholder
    asm volatile("mov %%esp, %0" : "=r"(currentESP));

    static IA32TaskStateSegmentManager tssManager(currentESP);

    _tssManager = &tssManager;

    // initialize SYSENTER MSRs for fast system call entry
    static_cast<IA32CPUDriver*>(context->CPU)->WriteMSR(sysenterCS, 0x08);
    static_cast<IA32CPUDriver*>(context->CPU)->WriteMSR(sysenterESP, currentESP);
    static_cast<IA32CPUDriver*>(context->CPU)->WriteMSR(
      sysenterEIP,
      reinterpret_cast<UInt32>(SYSENTER)
    );

    context->Log->Trace(
      "SYSENTER MSRs: CS=%x, ESP=%p, EIP=%p",
      0x08,
      currentESP,
      reinterpret_cast<UInt32>(SYSENTER)
    );

    static IA32ThreadContextManager threadContextManager(
      static_cast<IA32CPUDriver*>(context->CPU),
      _tssManager,
      context->KernelAddressSpace,
      context->MaydayHandler
    );

    context->ThreadContextManager = &threadContextManager;
  }

  void PCKernelInitializer::_initializeUserMode(KernelContext* context) {
    static Arch::IA32::UserMode::IA32UserModeManager userModeManager(_tssManager);

    context->UserModeManager = &userModeManager;
  }

  void PCKernelInitializer::_initializeDevices(KernelContext* context) {
    static MC146818Driver rtc;
    static ChipsetDriver chipset(_pci);
    static Intel8237ADriver dma;

    _platformDeviceCount = 0;

    // use S3 ViRGE if available, otherwise VESA
    if (_s3Virge) {
      _s3Virge->GetDevice().State = DeviceState::Active;
      _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(
        _s3Virge
      );
    } else {
      _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(_vesa);
    }

    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(
      context->Timer
    );
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(
      context->InterruptController
    );
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(_serial);
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(
      context->CPU
    );
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(&rtc);
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(&chipset);
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(&dma);
    _platformDevices[_platformDeviceCount++] = static_cast<IDriver*>(_pci);

    context->Drivers = PointerList<IDriver*>(
      _platformDevices,
      _platformDeviceCount
    );
  }
}
