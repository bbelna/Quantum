/**
 * @file Kernel/Concurrency/ProcessManager.cpp
 * @brief Implements @ref @QKrnl::Concurrency::ProcessManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/Cast.hpp>

#include <KernelLog.hpp>
#include <IPC/IPCPort.hpp>
#include <IPC/IPCPortRepository.hpp>
#include <Memory/IAddressSpaceAllocator.hpp>
#include <Memory/SharedBufferRepository.hpp>
#include <Memory/SharedBufferResource.hpp>
#include <Memory/StackManager.hpp>
#include <Resources/KernelResourceManager.hpp>

#include "ProcessManager.hpp"

namespace Quantum::Kernel::Concurrency {
  ProcessManager::ProcessManager(
    ThreadManager& threadManager,
    IAddressSpaceAllocator& addressSpaceAllocator,
    IMaydayHandler& maydayHandler,
    IAddressSpace& kernelAddressSpace,
    IMemoryAllocator& memoryAllocator,
    IMemoryMapper& memoryMapper,
    StackManager& stacks,
    KernelResourceManager& resources,
    IPCPortRepository* ipcRegistry,
    SharedBufferRepository* SharedBufferRepository,
    UIntPtr kernelBase
  ) :
    _threadManager(threadManager),
    _addressSpaceAllocator(addressSpaceAllocator),
    _maydayHandler(maydayHandler),
    _kernelAddressSpace(kernelAddressSpace),
    _memoryAllocator(memoryAllocator),
    _memoryMapper(memoryMapper),
    _stacks(stacks),
    _resources(resources),
    _ipcRegistry(ipcRegistry),
    _SharedBufferRepository(SharedBufferRepository),
    _kernelBase(kernelBase),
    _kernelProcess(nullptr),
    _processes{},
    _processList(nullptr),
    _processIDs(0),
    _lock()
  {
    // create the kernel process (PID 0)
    _kernelProcess = Create("Kernel.qbn", nullptr);

    if (!_kernelProcess) {
      MAYDAY("Failed to create kernel process");
    }

    // the kernel process uses the kernel address space
    _kernelProcess->AddressSpace = &_kernelAddressSpace;
    _kernelProcess->State = ProcessState::Running;
  }

  void ProcessManager::SetIPCDependencies(
    IPCPortRepository* ipcRegistry,
    SharedBufferRepository* sharedBufferRepository
  ) {
    _ipcRegistry = ipcRegistry;
    _SharedBufferRepository = sharedBufferRepository;
  }

  ProcessManager::~ProcessManager() {
    // clean up all processes
    for (
      Size processIndex = 0;
      processIndex < PROCESS_MAX_COUNT;
      processIndex++
    ) {
      if (_processes[processIndex]) {
        _cleanup(_processes[processIndex]);

        _processes[processIndex] = nullptr;
      }
    }
  }

  Process* ProcessManager::Create(
    const char* name,
    Process* parent
  ) {
    constexpr Size pdeCoverage = 4 * 1024 * 1024;

    _lock.Acquire();

    // allocate process ID
    ProcessID id = _allocateID();

    if (id >= PROCESS_MAX_COUNT) {
      _lock.Release();

      KLOG_ERROR(
        "Failed to allocate PID %u: out of process IDs",
        id
      );

      return nullptr;
    }

    // allocate process structure
    Process* process = new Process();

    if (!process) {
      _freeID(id);
      _lock.Release();

      KLOG_ERROR(
        "Failed to allocate process structure for PID %u",
        id
      );

      return nullptr;
    }

    // initialize process structure
    process->ID = id;

    CString::Copy(
      name
        ? name
        : "unnamed",
      process->Name,
      sizeof(process->Name)
    );

    process->State = ProcessState::Created;
    process->Parent = parent;
    process->AddressSpace = nullptr;
    process->MainThread = nullptr;
    process->ThreadCount = 0;
    process->ExitCode = 0;
    process->Next = nullptr;
    process->Previous = nullptr;
    process->FirstChild = nullptr;
    process->NextSibling = nullptr;
    process->Permissions = ProcessPermissions::None;

    // allocate address space for non-kernel processes
    if (id != 0) {
      process->AddressSpace = _addressSpaceAllocator.Allocate();

      if (!process->AddressSpace) {
        delete process;

        _freeID(id);
        _lock.Release();

        KLOG_ERROR("Failed to allocate address space for PID %u", id);

        return nullptr;
      }

      // initialize the child's address space map with sentinel entries only;
      // do NOT copy the kernel's map, that would shallow-copy the red-black
      // tree and share nodes between kernel and child, causing corruption on
      // insert/rotate; kernel PDEs are already shared via page tables

      // identity map guard: reserve the identity-mapped kernel memory region
      // so that user-space allocations never land in the process address range
      // the kernel uses for direct kernel memory access; without this,
      // Memory_Allocate would overwrite identity-mapped PTEs and corrupt the
      // kernel's view of kernel memory (page tables, page directories, etc.)
      //
      // round up to PDE boundary (4MB) so that reserved/ACPI regions between
      // or after usable E820 entries are also covered; the identity map
      // populates entire PDEs, not individual pages
      Size identityMapSize = AlignUp(
        _memoryAllocator.GetManagedBytes(),
        pdeCoverage
      );

      process->AddressSpaceMap.Insert(
        MemoryMapping {
          process->AddressSpace,
          MemoryBlock {
            0,
            identityMapSize
          },
          MemoryBlock {},
          MemoryMappingFlags {
            MemoryMappingPermissions::None,
            MemoryMappingCache::Default,
            MemoryMappingOptions::Guard
          },
          MemoryRegionType::Guard
        }
      );

      // kernel area sentinel: prevent user allocations above kernel base
      UIntPtr kernelBase = _kernelBase;

      process->AddressSpaceMap.Insert(
        MemoryMapping {
          process->AddressSpace,
          MemoryBlock {
            kernelBase,
            0xFFFFFFFFU - kernelBase
          },
          MemoryBlock {},
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
    } else {
      process->AddressSpace = &_kernelAddressSpace;
    }

    // add to parent's child list
    if (parent) {
      process->NextSibling = parent->FirstChild;
      parent->FirstChild = process;
    }

    // register in process table and list
    _processes[id] = process;

    _addToList(process);

    _lock.Release();

    KLOG_TRACE(
      "Created PID %u (%s) with parent PID %u",
      process->ID,
      process->Name,
      parent ? parent->ID : 0
    );

    return process;
  }

  Process* ProcessManager::Spawn(
    const char* name,
    UIntPtr entryPoint,
    Process* parent,
    Size argumentCount,
    const char* argumentData,
    Size argumentDataSize,
    UIntPtr sourceBase,
    Size sourceSizeInBytes,
    UIntPtr targetBase,
    Size segmentCount,
    const ProcessSpawnSegment* segments,
    UInt8 streamCount,
    const SharedBufferID* streamBufferIDs
  ) {
    static constexpr Size maxArguments = 64;
    static constexpr UIntPtr streamTableAddress = 0x0FFFF000;

    Process* process = Create(name, parent);

    if (!process) return nullptr;

    // if a source binary was specified, remap its kernel pages into the
    // child's address space at the target base (or the page-aligned entry
    // point if no explicit target was given)
    if (sourceBase != 0 && sourceSizeInBytes > 0) {
      Size blockSize = _memoryAllocator.GetBlockSize();
      Size blockCount = AlignUp<Size>(sourceSizeInBytes, blockSize) / blockSize;

      if (targetBase == 0) {
        targetBase = entryPoint & ~(blockSize - 1);
      }

      for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
        MemoryBlock sourceBlock {
          sourceBase + blockIndex * blockSize,
          blockSize
        };

        // resolve the kernel page backing the source process address;
        // use the parent's address space because the source buffer lives in
        // the parent's private heap (each process has independent low-memory
        // page tables after deep-copy, so the kernel's identity map points to
        // different kernel frames than the parent's user-space allocations)
        IAddressSpace& sourceSpace = parent
          ? *parent->AddressSpace
          : _kernelAddressSpace;

        MemoryBlock kernelBlock = _memoryMapper.Resolve(
          sourceSpace,
          sourceBlock
        );

        if (kernelBlock.Base == 0) {
          KLOG_ERROR(
            "Failed to resolve kernel page for source %p",
            sourceBlock.Base
          );

          Terminate(process, -1);

          return nullptr;
        }

        MemoryBlock targetBlock {
          targetBase + blockIndex * blockSize,
          blockSize
        };

        // determine per-page permissions from segment descriptors
        MemoryMappingPermissions permissions
          = MemoryMappingPermissions::Read
          | MemoryMappingPermissions::User;

        if (segmentCount > 0 && segments) {
          Size pageOffset = blockIndex * blockSize;
          Size pageEnd = pageOffset + blockSize;

          for (
            Size segmentIndex = 0;
            segmentIndex < segmentCount;
            segmentIndex++
          ) {
            Size segmentStart = segments[segmentIndex].OffsetInBytes;
            Size segmentEnd = segmentStart + segments[segmentIndex].SizeInBytes;

            if (pageOffset < segmentEnd && pageEnd > segmentStart) {
              if (segments[segmentIndex].Permissions & 2) {
                permissions
                  = permissions
                  | MemoryMappingPermissions::Write;
              }

              if (segments[segmentIndex].Permissions & 4) {
                permissions
                  = permissions
                  | MemoryMappingPermissions::Execute;
              }
            }
          }
        } else {
          // no segment info, default to read+write+user
          permissions
            = permissions
            | MemoryMappingPermissions::Write;
        }

        MemoryMappingFlags flags {
          permissions,
          MemoryMappingCache::Default,
          MemoryMappingOptions::None
        };

        // map the same kernel page at the target address in the child
        if (
          _memoryMapper.Map(
            *process->AddressSpace,
            targetBlock,
            kernelBlock,
            flags
          ).SizeInBytes == 0
        ) {
          KLOG_ERROR(
            "Failed to map binary page at %p",
            targetBlock.Base
          );

          Terminate(process, -1);

          return nullptr;
        }

        // retain the shared kernel page so it survives the parent freeing
        // its source buffer; the parent's Free drops one ref, the child's
        // page directory walk during cleanup drops the other
        _memoryAllocator.Retain(kernelBlock);

        // record the mapping in the child's address space map so the
        // kernel can track which virtual regions are in use
        process->AddressSpaceMap.Insert(
          MemoryMapping {
            process->AddressSpace,
            targetBlock,
            kernelBlock,
            flags,
            MemoryRegionType::Code
          }
        );
      }

      process->TotalBlockCount += blockCount;

      KLOG_TRACE(
        "Mapped %u bytes from %p to %p in PID %u (%s)",
        sourceSizeInBytes,
        sourceBase,
        targetBase,
        process->ID,
        name
      );
    }

    Stack* stack = _stacks.Create(
      *process->AddressSpace,
      THREAD_STACK_SIZE,
      MemoryMappingPermissions::User
    );

    if (!stack || stack->Base == 0) {
      Terminate(process, -1);

      KLOG_ERROR("Stack allocation failed");

      return nullptr;
    }

    // record occupied process regions in the process' address space map so
    // that Memory_Allocate (FindAvailableBlock) knows these ranges are taken
    // and returns a valid non-zero address for heap allocations
    Size blockSize = _memoryAllocator.GetBlockSize();
    Size stackBlockCount = stack->SizeInBytes / blockSize;

    for (
      Size stackBlockIndex = 0;
      stackBlockIndex < stackBlockCount;
      stackBlockIndex++
    ) {
      process->AddressSpaceMap.Insert(
        MemoryMapping {
          process->AddressSpace,
          MemoryBlock {
            stack->Base + stackBlockIndex * blockSize,
            blockSize
          },
          MemoryBlock {},
          MemoryMappingFlags {
            MemoryMappingPermissions::Read
              | MemoryMappingPermissions::Write
              | MemoryMappingPermissions::User,
            MemoryMappingCache::Default,
            MemoryMappingOptions::None
          },
          MemoryRegionType::Stack
        }
      );
    }

    process->TotalBlockCount += stackBlockCount;

    // initialize the stack with command-line arguments
    //
    // CRT0 expects this layout (low address = ESP, high address = top):
    //   [argc] [argv[0]] [argv[1]] ... [argv[N-1]] [0 (envp)]
    //
    // since the stack grows down and Push places values at decreasing
    // addresses, we push in reverse order:
    //   1. string data        (highest addresses, pushed first)
    //   2. envp null terminator
    //   3. argv[N-1] ... argv[0] pointers into the string data
    //   4. argc               (lowest address = ESP)
    if (argumentCount > 0 && (!argumentData || argumentDataSize == 0)) {
      Terminate(process, -1);

      KLOG_ERROR(
        "Argument count is %zu but argument data is null",
        argumentCount
      );

      return nullptr;
    }

    // push the packed string data onto the stack; the stack address IS the
    // child's user-space address, so argv pointers can reference it directly
    UIntPtr argumentAddresses[maxArguments] = {};

    if (argumentCount > maxArguments) argumentCount = maxArguments;

    if (argumentCount > 0 && argumentData && argumentDataSize > 0) {
      // align the stack to 4 bytes before pushing string data so that
      // subsequent pointer pushes stay word-aligned
      Size alignedDataSize = (argumentDataSize + 3) & ~static_cast<Size>(3);
      Size paddingBytes = alignedDataSize - argumentDataSize;

      // reserve space for alignment padding first
      if (paddingBytes > 0) stack->Top -= paddingBytes;

      if (!stack->PushBytes(argumentData, argumentDataSize)) {
        Terminate(process, -1);

        KLOG_ERROR("Failed to push argument string data");

        return nullptr;
      }

      // stack->Top now points to the start of the string block in the
      // child's address space; walk the packed strings to record each
      // argument's address
      UIntPtr stringBase = stack->Top;
      Size currentArgument = 0;
      Size offset = 0;

      while (offset < argumentDataSize && currentArgument < argumentCount) {
        argumentAddresses[currentArgument] = stringBase + offset;
        ++currentArgument;

        // advance past the null terminator to the next string
        while (offset < argumentDataSize && argumentData[offset] != '\0') {
          ++offset;
        }

        ++offset;
      }
    }

    // push the envp null terminator (ends up at highest address after args)
    stack->Push<UInt32>(0);

    // push argv pointers in reverse order so argv[0] is closest to ESP
    for (
      Size argumentIndex = 0;
      argumentIndex < argumentCount;
      argumentIndex++
    ) {
      UIntPtr address = argumentAddresses[argumentCount - 1 - argumentIndex];

      if (!stack->Push(address)) {
        Terminate(process, -1);

        KLOG_ERROR("Failed to push argument pointer %u", argumentIndex);

        return nullptr;
      }
    }

    // push argc last (ends up at lowest address = ESP)
    if (!stack->Push(argumentCount)) {
      Terminate(process, -1);

      KLOG_ERROR("Failed to push argument count");

      return nullptr;
    }

    blockSize = _memoryAllocator.GetBlockSize();

    // always map the process stream table page so CRT0 can safely read
    // it; populate with buffer IDs when the parent provides streams
    MemoryBlock streamBlock = _memoryAllocator.Allocate(
      MemoryBlockTag::ProcessHeap
    );

    if (streamBlock.Base == 0) {
      Terminate(process, -1);

      KLOG_ERROR("Failed to allocate stream table page");

      return nullptr;
    }

    // zero the page and write the stream table
    Byte::Zero(reinterpret_cast<void*>(streamBlock.Base), blockSize);

    UInt8* streamTable = reinterpret_cast<UInt8*>(streamBlock.Base);
    UInt8 effectiveStreamCount = (streamCount > 3) ? 3 : streamCount;

    if (streamBufferIDs) {
      streamTable[0] = effectiveStreamCount;

      // Reserved[3] already zeroed

      // 3 x UInt32 starting at offset 4
      UInt32* entries = reinterpret_cast<UInt32*>(streamTable + 4);

      for (
        UInt8 streamIndex = 0;
        streamIndex < effectiveStreamCount;
        streamIndex++
      ) entries[streamIndex] = streamBufferIDs[streamIndex];
    }

    MemoryBlock streamProcessBlock {
      streamTableAddress,
      blockSize
    };

    MemoryMappingFlags streamFlags {
      MemoryMappingPermissions::Read | MemoryMappingPermissions::User,
      MemoryMappingCache::Default,
      MemoryMappingOptions::None
    };

    if (
      _memoryMapper.Map(
        *process->AddressSpace,
        streamProcessBlock,
        streamBlock,
        streamFlags
      ).SizeInBytes == 0
    ) {
      _memoryAllocator.Free(streamBlock);

      Terminate(process, -1);

      KLOG_ERROR("Failed to map stream table page");

      return nullptr;
    }

    process->AddressSpaceMap.Insert(
      MemoryMapping {
        process->AddressSpace,
        streamProcessBlock,
        streamBlock,
        streamFlags,
        MemoryRegionType::Shared
      }
    );

    process->TotalBlockCount += 1;

    if (streamTable[0] > 0) {
      KLOG_TRACE(
        "Mapped stream table (%u streams) at %p for PID %u",
        streamTable[0],
        streamTableAddress,
        process->ID
      );
    }

    // create the main thread for this process
    Thread* mainThread = CreateUserThread(
      process,
      process->Name,
      entryPoint,
      stack->Top,
      ThreadPriority::UserNormal
    );

    if (!mainThread) {
      Terminate(process, -1);

      KLOG_ERROR("Failed to create process thread");

      return nullptr;
    }

    mainThread->UserStack = stack;
    process->State = ProcessState::Running;

    KLOG_TRACE(
      "Spawned \"%s\" (PID %u) with parent PID %u",
      process->Name,
      process->ID,
      process->Parent ? process->Parent->ID : 0
    );

    return process;
  }

  void ProcessManager::Terminate(Process* process, Int32 exitCode) {
    if (!process) return;

    _lock.Acquire();

    // guard against double-termination
    if (
      process->State == ProcessState::Terminating ||
      process->State == ProcessState::Zombie
    ) {
      _lock.Release();

      return;
    }

    process->ExitCode = exitCode;
    process->State = ProcessState::Terminating;

    // terminate all threads belonging to this process (except the current
    // one, which is handled by the caller's Threads->Terminate call)
    _threadManager.TerminateByProcess(process, exitCode);

    // reparent children to the kernel process
    Process* child = process->FirstChild;

    while (child) {
      Process* next = child->NextSibling;

      child->Parent = _kernelProcess;
      child->NextSibling = _kernelProcess->FirstChild;

      _kernelProcess->FirstChild = child;

      child = next;
    }

    process->FirstChild = nullptr;

    // remove from parent's child list
    if (process->Parent) {
      Process* previous = nullptr;
      Process* current = process->Parent->FirstChild;

      while (current) {
        if (current == process) {
          if (previous) {
            previous->NextSibling = current->NextSibling;
          } else {
            process->Parent->FirstChild = current->NextSibling;
          }

          break;
        }

        previous = current;
        current = current->NextSibling;
      }
    }

    process->State = ProcessState::Zombie;

    // wake all threads waiting for this process to exit
    LinkedNode<Thread*>* node;

    while ((node = process->WaitQueue.Pop()) != nullptr) {
      Thread* waiter = node->GetValue();

      delete node;

      if (waiter) {
        _lock.Release();
        _threadManager.Resume(waiter);
        _lock.Acquire();
      }
    }

    _lock.Release();

    KLOG_TRACE(
      "Terminated PID %u (%s) with exit code %d",
      process->ID,
      process->Name,
      exitCode
    );
  }

  Process* ProcessManager::GetByID(ProcessID id) {
    if (id >= PROCESS_MAX_COUNT) return nullptr;

    return _processes[id];
  }

  Process* ProcessManager::GetCurrent() {
    Thread* currentThread = _threadManager.GetCurrent();

    return currentThread && currentThread->OwnerProcess
      ? currentThread->OwnerProcess
      : _kernelProcess;
  }

  Thread* ProcessManager::CreateThread(
    Process* process,
    const char* name,
    KernelThreadEntryPoint entryPoint,
    void* argument,
    ThreadPriority priority
  ) {
    if (process) {
      // determine flags based on process
      ThreadFlags flags = ThreadFlags::Kernel;

      if (process != _kernelProcess) {
        flags = ThreadFlags::User | ThreadFlags::Joinable;
      }

      // create the thread with the process' address space
      // TODO: stack sizing?
      Thread* thread = _threadManager.Create(
        name,
        entryPoint,
        argument,
        priority,
        flags,
        ThreadStackSize,
        process->AddressSpace
      );

      if (thread) {
        // associate thread with process
        thread->OwnerProcess = process;

        _lock.Acquire();

        // set as main thread if this is the first
        if (!process->MainThread) {
          process->MainThread = thread;
        }

        process->ThreadCount++;

        _lock.Release();

        return thread;
      } else {
        return nullptr;
      }
    } else {
      KLOG_WARNING("ProcessManager::CreateThread received null process");

      return nullptr;
    }
  }

  Thread* ProcessManager::CreateUserThread(
    Process* process,
    const char* name,
    UIntPtr entryPoint,
    UIntPtr stackTop,
    ThreadPriority priority
  ) {
    if (!process) {
      KLOG_ERROR("null process");

      return nullptr;
    }

    if (process == _kernelProcess) {
      KLOG_ERROR("Cannot create user thread in kernel process");

      return nullptr;
    }

    // create the user mode thread
    Thread* thread = _threadManager.CreateUserModeThread(
      name,
      entryPoint,
      stackTop,
      priority,
      process->AddressSpace
    );

    if (!thread) return nullptr;

    // associate thread with process
    thread->OwnerProcess = process;

    _lock.Acquire();

    // set as main thread if this is the first
    if (!process->MainThread) process->MainThread = thread;

    process->ThreadCount++;

    _lock.Release();

    return thread;
  }

  bool ProcessManager::HasPermissions(
    ProcessID pid,
    ProcessPermissions required
  ) {
    Process* process = GetByID(pid);

    if (!process) return false;

    return (process->Permissions & required) == required;
  }

  Size ProcessManager::EnumerateProcesses(
    ProcessInfo* buffer,
    Size maxCount
  ) {
    _lock.Acquire();

    Size count = 0;

    for (
      Process* process = _processList;
      process && count < maxCount;
      process = process->Next
    ) {
      buffer[count].ID = process->ID;

      CString::Copy(process->Name, buffer[count].Name, 64);

      buffer[count].State = static_cast<UInt8>(process->State);
      buffer[count].ThreadCount = process->ThreadCount;
      buffer[count].HeapPageCount = process->HeapBlockCount;
      buffer[count].TotalPageCount = process->TotalBlockCount;

      count++;
    }

    _lock.Release();

    return count;
  }

  Size ProcessManager::GetTotalUserPages() const {
    // NOTE: casting away const on _lock is acceptable here because the
    // spinlock protects the linked list traversal, not the const-ness of
    // the data; the alternative would be making _lock mutable
    const_cast<Spinlock<UInt32>&>(_lock).Acquire();

    Size total = 0;

    for (
      Process* process = _processList;
      process;
      process = process->Next
    ) total += process->TotalBlockCount;

    const_cast<Spinlock<UInt32>&>(_lock).Release();

    return total;
  }

  ProcessID ProcessManager::_allocateID() {
    ProcessID id;

    if (!_processIDs.Allocate(id)) return PROCESS_MAX_COUNT;

    return id;
  }

  void ProcessManager::_freeID(ProcessID id) {
    _processIDs.Free(id);
  }

  void ProcessManager::ReapZombies() {
    _lock.Acquire();

    Process* current = _processList;

    while (current) {
      Process* next = current->Next;

      if (
        current->State == ProcessState::Zombie &&
        current->ID != 0 &&
        current->ThreadCount == 0
      ) {
        _lock.Release();

        _cleanup(current);

        _lock.Acquire();
      }

      current = next;
    }

    _lock.Release();
  }

  void ProcessManager::_cleanup(Process* process) {
    if (!process) return;

    KLOG_TRACE(
      "PID %u (%s), ThreadCount=%u",
      process->ID,
      process->Name,
      process->ThreadCount
    );

    // clean up all resources owned by this process: IPC ports,
    // interrupts, displays, and shared buffers
    _resources.RemoveByOwner(
      process->ID,
      [this](KernelResourceBase* base) {
        if (base->Type == KernelResourceType::IPCPort) {
          IPCPortResource* resource = static_cast<IPCPortResource*>(base);

          // save fields BEFORE DecrementRefCount, it self-deletes when
          // the count reaches zero, so any access after that is a
          // use-after-free
          IPCPortID portID = resource->ObjectID;
          bool isManager = Enum::HasAnyFlag(
            resource->Rights,
            IPCPortRights::Manage
          );

          if (
            resource->DecrementRefCount() == 0 &&
            isManager &&
            _ipcRegistry
          ) _ipcRegistry->RemoveByID(portID);
        } else if (base->Type == KernelResourceType::SharedBuffer) {
          SharedBufferResource* resource = Cast::As<SharedBufferResource>(base);

          // save fields BEFORE DecrementRefCount
          SharedBufferID bufferID = resource->ObjectID;
          bool isAttach = Enum::HasAnyFlag(
            resource->Rights,
            SharedBufferRights::Attach
          );

          resource->DecrementRefCount();

          // if this was an Attach reference, the cleanup is the same as if the
          // process had called Detach: decrement the AttachCount on the shared
          // buffer, and if it reaches zero and the buffer isn't pinned, free
          // all kernel blocks and remove the buffer from the registry; the
          // parent's cleanup will also do this, so the buffer is freed when
          // both parent and child have detached or terminated, whichever
          // happens last
          if (_SharedBufferRepository && isAttach) {
            SharedBuffer* buffer = _SharedBufferRepository->Find(bufferID);

            if (buffer) {
              if (buffer->AttachCount > 0) buffer->AttachCount--;

              if (buffer->AttachCount == 0 && !buffer->Pinned) {
                for (
                  Size blockIndex = 0;
                  blockIndex < buffer->BlockCount;
                  ++blockIndex
                ) _memoryAllocator.Free(buffer->Blocks[blockIndex]);

                _SharedBufferRepository->Remove(bufferID);
              }
            }
          }
        } else {
          base->DecrementRefCount();
        }
      }
    );

    // free address space; PageDirectoryManager::Free() walks ALL
    // user-space page table entries and frees their kernel pages
    // (including demand-paged pages not recorded in ProcessAddressSpaceMap),
    // then frees the cloned page tables and directory itself.
    //
    // Spawn called Retain on each binary page (refcount 1 -> 2) so
    // the page survives the parent freeing its source buffer.  The
    // parent's buffer Free drops one reference; this PD walk drops the
    // other.  If the parent hasn't freed yet, the page goes to
    // refcount 1 here and the parent's eventual Free (or cleanup)
    // releases the last reference.
    if (process->AddressSpace && process->ID != 0) {
      _addressSpaceAllocator.Free(process->AddressSpace);
    } else if (process->ID != 0) {
      KLOG_WARNING(
        "PID %u (%s): no address space to free during cleanup",
        process->ID,
        process->Name
      );
    }

    // 3. remove from global list and process table
    _removeFromList(process);

    if (process->ID < PROCESS_MAX_COUNT) _processes[process->ID] = nullptr;

    _freeID(process->ID);

    // delete the process struct; triggers ~ProcessAddressSpaceMap
    // -> ~RedBlackTree which frees all RB-tree nodes
    delete process;
  }

  void ProcessManager::_addToList(Process* process) {
    process->Next = _processList;
    process->Previous = nullptr;

    if (_processList) _processList->Previous = process;

    _processList = process;
  }

  void ProcessManager::_removeFromList(Process* process) {
    if (process->Previous) {
      process->Previous->Next = process->Next;
    } else {
      _processList = process->Next;
    }

    if (process->Next) process->Next->Previous = process->Previous;

    process->Next = nullptr;
    process->Previous = nullptr;
  }
}
