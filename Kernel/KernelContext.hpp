/**
 * @file Kernel/KernelContext.hpp
 * @brief Declares @ref @QKrnl::KernelContext.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Drivers/Timers/ITimerDriver.hpp"
#include "KernelConstants.hpp"
#include "KernelTypes.hpp"
#include "Memory/StackManagerConfiguration.hpp"

namespace Quantum::Kernel {
  /**
   * @brief Represents the kernel state and holds pointers to all kernel
   *        dependencies.
   */
  struct KernelContext {
    /**
     * @brief The kernel mayday handler.
     * @see @ref IMaydayHandler, @ref Arch::IA32::IA32MaydayHandler
     *
     * Implemented by either the @ref Arch or @ref Platform layer, depending on
     * which is better able to handle mayday conditions.
     */
    IMaydayHandler* MaydayHandler = nullptr;

    /**
     * @brief The kernel's current @ref LogLevel.
     * @see @ref LogLevel, @ref SpinlockLog, @ref ScreenLogSink
     */
    LogLevel LogLevel = LogLevel::Info;

    /**
     * @brief The base virtual address of the kernel.
     *
     * Set by the platform initializer from architecture-specific linker
     * symbols. Used by the process manager to guard the kernel region in
     * per-process address space maps.
     */
    UIntPtr KernelBase = 0;

    /**
     * @brief The base virtual address where the initial process is mapped.
     *
     * Set by the platform initializer. Used by the kernel during boot to
     * map the initial process image into its address space.
     */
    UIntPtr InitialProcessAddress = 0;

    /**
     * @brief The base address of the kernel's stack area.
     */
    UIntPtr StackAreaBase = 0;

    /**
     * @brief The size of the kernel's stack area in bytes.
     */
    Size StackAreaSizeInBytes = 0;

    /**
     * @brief The kernel memory allocator.
     */
    IMemoryAllocator* MemoryAllocator = nullptr;

    /**
     * @brief The kernel memory mapper.
     */
    IMemoryMapper* MemoryMapper = nullptr;

    /**
     * @brief The kernel interrupt manager.
     */
    IInterruptManager<InterruptVector>* InterruptManager = nullptr;

    /**
     * @brief The kernel interrupt controller (PIC).
     */
    IInterruptControllerDriver<InterruptVector>* InterruptController
      = nullptr;

    /**
     * @brief The kernel heap allocator.
     */
    HeapAllocator* HeapAllocator = nullptr;

    /**
     * @brief The kernel's address space.
     */
    IAddressSpace* KernelAddressSpace = nullptr;

    /**
     * @brief The kernel address space map.
     *
     * This tracks occupied regions of the kernel's address space, along with
     * their flags and semantic region type.  The kernel address space is
     * identity-mapped, so no translation/mapping is needed.
     */
    KernelAddressSpaceMap* KernelAddressSpaceMap = nullptr;

    /**
     * @brief The kernel address translator.
     */
    IAddressTranslator* AddressTranslator = nullptr;

    /**
     * @brief The kernel address space allocator.
     */
    IAddressSpaceAllocator* AddressSpaceAllocator = nullptr;

    /**
     * @brief The @ref StackManager for allocating and managing @ref Thread
     *        @ref Stack instances.
     */
    StackManager* Stacks = nullptr;

    /**
     * @brief The kernel thread context manager.
     */
    IThreadContextManager* ThreadContextManager = nullptr;

    /**
     * @brief The kernel thread manager.
     */
    ThreadManager* ThreadManager = nullptr;

    /**
     * @brief The kernel scheduler.
     */
    HybridCFSBitmapScheduler* Scheduler = nullptr;

    /**
     * @brief The kernel process manager.
     */
    ProcessManager* ProcessesManager = nullptr;

    /**
     * @brief The kernel user mode manager.
     */
    IUserModeManager* UserModeManager = nullptr;

    /**
     * @brief The timer's device driver.
     */
    ITimerDriver* Timer = nullptr;

    /**
     * @brief The CPU's device driver.
     */
    ICPUDriver* CPU = nullptr;

    /**
     * @brief The kernel IPC port repository.
     */
    IPCPortRepository* IPCPortRepository = nullptr;

    /**
     * @brief The kernel IPC port manager.
     */
    IPCPortService* IPCPortService = nullptr;

    /**
     * @brief The kernel resource manager.
     */
    KernelResourceManager* KernelResourceManager = nullptr;

    /**
     * @brief The kernel shared memory buffer registry.
     */
    SharedBufferRepository* SharedBufferRepository = nullptr;

    /**
     * @brief The kernel futex manager for user-space synchronization.
     */
    FutexManager* FutexManager = nullptr;

    /**
     * @brief Object pool for IPC message envelopes.
     */
    ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>* IPCMessagePool = nullptr;

    /**
     * @brief Object pool for IPC port structures.
     */
    ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>* IPCPortPool = nullptr;

    /**
     * @brief Object pool for shared buffer descriptors.
     */
    ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>* SharedBufferPool
      = nullptr;

    /**
     * @brief The kernel memory pressure monitor.
     */
    MemoryPressureMonitor* MemoryPressureMonitor = nullptr;

    /**
     * @brief The @ref @QKrnl @ref SpinlockLog instance.
     */
    SpinlockLog* Log = nullptr;

    /**
     * @brief @ref PointerList of @QKrnl-level @ref IDriver instances.
     */
    PointerList<IDriver*> Drivers;

    /**
     * @brief The kernel stack manager configuration provider.
     *
     * This is set by the architecture-specific initialization code to
     * provide the stack manager configuration derived from
     * architecture-specific memory layout constants.
     */
    IProvider<StackManagerConfiguration>* StackManagerConfigurationProvider
      = nullptr;
  };
}
