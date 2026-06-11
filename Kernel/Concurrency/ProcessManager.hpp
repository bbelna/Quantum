/**
 * @file Kernel/Concurrency/ProcessManager.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ProcessManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/ProcessInfo.hpp>
#include <Concurrency/ProcessSpawnSegment.hpp>

#include "Process.hpp"
#include "ProcessPermissions.hpp"
#include "Spinlock.hpp"
#include "ThreadManager.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Manages @ref Process lifecycle, creation, termination, and lookup.
   * @note All public methods are internally synchronized via a @ref Spinlock
   *       unless otherwise noted.
   *
   * Owns the global @ref Process table (up to @ref PROCESS_MAX_COUNT entries)
   * and coordinates with @ref ThreadManager for @ref Thread creation,
   * @ref IAddressSpaceAllocator for @ref Process address spaces, and
   * @ref StackManager for @ref Thread @ref Stack allocation.
   */
  class ProcessManager {
    public:
      /**
       * @brief
       *   Creates a new @ref ProcessManager.
       * @param threadManager
       *   Reference to the @ref ThreadManager.
       * @param addressSpaceAllocator
       *   Reference to the @ref IAddressSpaceAllocator.
       * @param maydayHandler
       *   Reference to a concrete implementation of @ref IMaydayHandler.
       * @param kernelAddressSpace
       *   Reference to the kernel @ref IAddressSpace.
       * @param memoryAllocator
       *   Reference to a concrete implementation of
       *   @ref IMemoryAllocator.
       * @param memoryMapper
       *   Reference to a concrete implementation of @ref IMemoryMapper.
       * @param stacks
       *   Reference to the @ref StackManager.
       * @param resources
       *   Reference to the @ref KernelResourceManager.
       * @param ipcRegistry
       *   Pointer to the @ref IPCPortRepository.
       * @param SharedBufferRepository
       *   Pointer to the @ref SharedBufferRepository.
       */
      explicit ProcessManager(
        ThreadManager& threadManager,
        IAddressSpaceAllocator& addressSpaceAllocator,
        IMaydayHandler& maydayHandler,
        IAddressSpace& kernelAddressSpace,
        IMemoryAllocator& memoryAllocator,
        IMemoryMapper& memoryMapper,
        StackManager& stackManager,
        KernelResourceManager& resourceManager,
        IPCPortRepository* portRepository,
        SharedBufferRepository* sharedBufferRepository,
        UIntPtr kernelBase
      );

      /**
       * @brief Sets the IPC cleanup dependencies.
       * @param ipcRegistry Pointer to the @ref IPCPortRepository.
       * @param sharedBufferRepository Pointer to the @ref SharedBufferRepository.
       *
       * Called by the IPC module after initialization to wire up the
       * dependencies that @ref ProcessManager uses during process cleanup.
       * These are optional — if not set, process cleanup will skip
       * IPC resource reclamation.
       */
      void SetIPCDependencies(
        IPCPortRepository* ipcRegistry,
        ::Quantum::Kernel::Memory::SharedBufferRepository* sharedBufferRepository
      );

      /**
       * @brief Destroys the @ref ProcessManager.
       */
      ~ProcessManager();

      /**
       * @brief Creates a new empty @ref Process.
       * @param name The @ref Process name (truncated to
       *             @ref ProcessNameMaxLength).
       * @param parent The parent process, or `nullptr` for orphan processes.
       * @return Pointer to the new @ref Process, or `nullptr` if the process
       *        table is full or memory allocation fails.
       *
       * The returned @ref Process is in @ref ProcessState::Created and has no
       * @ref Thread. The caller must create at least one @ref Thread (via
       * @ref CreateThread or @ref CreateUserThread) and start it before the
       * @ref Process can run.
       */
      Process* Create(
        const char* name,
        Process* parent
      );

      /**
       * @brief Spawns a new @ref Process.
       * @param name
       *   Name of the new @ref Process.
       *   Truncated to @ref ProcessNameMaxLength.
       * @param entryPoint
       *   Address of the entry point in the new @ref Process
       *   @ref IAddressSpace.
       * @param parent
       *   Pointer to the parent @ref Process. Can be `nullptr`.
       * @param argumentCount
       *   Number of command-line arguments (`argc`).
       * @param argumentData
       *   Packed null-terminated argument strings (`argv`), or `nullptr` if
       *   @p argumentCount is zero.
       * @param argumentDataSizeInBytes
       *   Total size in bytes of @p argumentData.
       * @param sourceBase
       *   Base address of the source image to copy from. `0` if no flat copy is
       *   needed.
       * @param sourceSizeInBytes
       *   Size of the flat source region in bytes.
       * @param targetBase
       *   Base address in the new @ref Process @ref IAddressSpace where the
       *   flat source image is mapped.
       * @param segmentCount
       *   Number of explicit segments in @p segments.
       * @param segments
       *   Array of @ref ProcessSpawnSegment pointers for fine-grained mapping
       *   (overrides the flat copy when non-`null`).
       * @param streamCount
       *   Number of @ref SharedBufferID pointers in @p streamBufferIDs.
       * @param streamBufferIDs
       *   Array of @ref SharedBufferID pointers to map in the new
       *   @ref Process.
       * @return
       *   Pointer to the newly created @ref Process, or `nullptr` on failure.
       * @note
       *   The caller must hold no locks; this method acquires the @ref Process
       *   lock internally.
       *
       * Allocates a new @ref IAddressSpace, maps the code/data from the
       * parent's @ref IAddressSpace (or from physical blocks described by
       * @p segments), creates a main @ref Process @ref Thread at @p entryPoint,
       * and starts the new @ref Process.
       */
      Process* Spawn(
        const char* name,
        UIntPtr entryPoint,
        Process* parent,
        Size argumentCount,
        const char* argumentData,
        Size argumentDataSizeInBytes,
        UIntPtr sourceBase = 0,
        Size sourceSizeInBytes = 0,
        UIntPtr targetBase = 0,
        Size segmentCount = 0,
        const ProcessSpawnSegment* segments = nullptr,
        UInt8 streamCount = 0,
        const SharedBufferID* streamBufferIDs = nullptr
      );

      /**
       * @brief Terminates a @ref Process.
       * @param process Pointer to the @ref Process to terminate.
       * @param exitCode The @ref Process exit code.
       *
       * Transitions the @ref Process to @ref ProcessState::Terminating,
       * terminates every @ref Thread owned by the @ref Process, wakes any
       * @ref Thread instances blocked in the @ref Process::WaitQueue, and marks
       * the @ref Process as @ref ProcessState::Zombie for later reaping by
       * @ref ReapZombies.
       */
      void Terminate(
        Process* process,
        Int32 exitCode
      );

      /**
       * @brief Gets a @ref Process by its @ref ProcessID.
       * @param id The @ref ProcessID of the @ref Process to get.
       * @return Pointer to the @ref Process, or `nullptr` if not found.
       */
      Process* GetByID(ProcessID id);

      /**
       * @brief Gets the current @ref Process.
       * @return Pointer to the current @ref Process.
       */
      Process* GetCurrent();

      /**
       * @brief Creates a new @ref Process @ref Thread.
       * @param process The owning @ref Process.
       * @param name The name for the new @ref Process @ref Thread.
       * @param entryPoint The @ref ThreadEntryPoint for the new @ref Process
       *                   @ref Thread.
       * @param argument Opaque argument pointer passed to entry function.
       * @param priority @ref Thread priority.
       * @return Pointer to the new @ref Thread, or `nullptr` on failure.
       */
      Thread* CreateThread(
        Process* process,
        const char* name,
        KernelThreadEntryPoint entryPoint,
        void* argument,
        ThreadPriority priority
      );

      /**
       * @brief
       *   Creates a new user-mode thread in a @ref Process.
       * @param process
       *   The owning user-mode @ref Process.
       * @param name @ref Thread name.
       * @param entryPointAddress
       *   The entry point address in the @ref Process
       *   @ref Process::AddressSpace.
       * @param stackTop
       *   Top of the @ref Stack in the @ref Process
       *   @ref Process::AddressSpace.
       * @param priority @ref Thread priority.
       * @return Pointer to the new @ref Thread, or `nullptr` on failure.
       */
      Thread* CreateUserThread(
        Process* process,
        const char* name,
        UIntPtr entryPointAddress,
        UIntPtr stackTop,
        ThreadPriority priority
      );

      /**
       * @brief Checks if a @ref Process has the specified permissions.
       * @param pid The @ref ProcessID.
       * @param permissions The permissions to check.
       * @return `true` if the @ref Process has all specified permissions; `false`
       *         otherwise.
       */
      bool HasPermissions(
        ProcessID pid,
        ProcessPermissions permissions
      );

      /**
       * @brief Enumerates all running @ref Process instances into a buffer.
       * @param buffer Pointer to an array of
       *               @ref ::Quantum::Kernel::ABI::Process::ProcessInfo to fill.
       * @param maxCount Maximum number of entries to write.
       * @return The number of entries written.
       */
      Size EnumerateProcesses(
        ProcessInfo* buffer,
        Size maxCount
      );

      /**
       * @brief Reaps all zombie @ref Process instances, cleaning up their
       *        resources.
       * @note Should be called periodically (e.g. from the idle thread) to
       *       reclaim memory.
       *
       * Walks the @ref Process list and frees kernel objects in the address
       * space (e.g., @ref Stack, @ref IPCPort, @ref Process).
       */
      void ReapZombies();

      /**
       * @brief Gets the kernel @ref Process.
       * @return Pointer to the kernel @ref Process.
       */
      Process* GetKernelProcess() { return _kernelProcess; }

      /**
       * @brief Returns the sum of `TotalPageCount` across all user processes.
       * @return The total user process page count.
       */
      Size GetTotalUserPages() const;

    private:
      /**
       * @brief Reference to the @ref ThreadManager.
       */
      ThreadManager& _threadManager;

      /**
       * @brief Reference to a concrete implementation of
       *        @ref IAddressSpaceAllocator.
       */
      IAddressSpaceAllocator& _addressSpaceAllocator;

      /**
       * @brief Reference to a concrete implementation of
       *        @ref IMaydayHandler.
       */
      IMaydayHandler& _maydayHandler;

      /**
       * @brief Reference to the kernel @ref IAddressSpace.
       */
      IAddressSpace& _kernelAddressSpace;

      /**
       * @brief Reference to the @ref IMemoryAllocator.
       */
      IMemoryAllocator& _memoryAllocator;

      /**
       * @brief Reference to the @ref IMemoryMapper.
       */
      IMemoryMapper& _memoryMapper;

      /**
       * @brief Reference to the @ref StackManager.
       */
      StackManager& _stacks;

      /**
       * @brief Reference to the @ref KernelResourceManager.
       */
      KernelResourceManager& _resources;

      /**
       * @brief Pointer to the @ref IPCPortRepository.
       */
      IPCPortRepository* _ipcRegistry = nullptr;

      /**
       * @brief Pointer to the @ref SharedBufferRepository.
       */
      SharedBufferRepository* _SharedBufferRepository = nullptr;

      /**
       * @brief The base virtual address of the kernel.
       */
      UIntPtr _kernelBase;

      /**
       * @brief Pointer to the kernel @ref Process (PID `0`).
       */
      Process* _kernelProcess = nullptr;

      /**
       * @brief Array of all @ref Process instances indexed by @ref ProcessID.
       */
      Process* _processes[PROCESS_MAX_COUNT] = {};

      /**
       * @brief Pointer to the head of the global @ref Process list.
       */
      Process* _processList = nullptr;

      /**
       * @brief @ref IDAllocator for @ref ProcessID with free list reuse.
       */
      IDAllocator<ProcessID, PROCESS_MAX_COUNT> _processIDs;

      /**
       * @brief @ref Spinlock protecting @ref ProcessManager state.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Allocates a new @ref ProcessID.
       * @return The new @ref ProcessID, or @ref PROCESS_MAX_COUNT on failure.
       */
      ProcessID _allocateID();

      /**
       * @brief Frees a @ref ProcessID for reuse.
       * @param id The @ref ProcessID to free.
       */
      void _freeID(ProcessID id);

      /**
       * @brief Cleans up a terminated @ref Process instance's resources.
       * @param process Pointer to the @ref Process to clean up.
       */
      void _cleanup(Process* process);

      /**
       * @brief Adds a @ref Process to the global list.
       * @param process Pointer to the @ref Process to add.
       */
      void _addToList(Process* process);

      /**
       * @brief Removes a @ref Process from the global list.
       * @param process Pointer to the @ref Process to remove.
       */
      void _removeFromList(Process* process);
  };
}
