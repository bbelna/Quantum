/**
 * @file Kernel/Arch/IA32/Interrupts/SystemCallHandler.cpp
 * @brief Implements @ref @QKrnlIA32::Interrupts::SystemCallHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Kernel/ABI.hpp>
#include <Quantum/Kernel/Memory/MemoryTypes.hpp>
#include <Quantum/Kernel/Concurrency/ProcessInfo.hpp>
#include <Quantum/Kernel/KernelOperation.hpp>

#include <Concurrency/AtomicMemoryOrder.hpp>
#include <Concurrency/FutexManager.hpp>
#include <Concurrency/IAtomic.hpp>
#include <Concurrency/IThreadContextManager.hpp>
#include <Concurrency/RefCount.hpp>
#include <Concurrency/RefCountScoped.hpp>
#include <Concurrency/HybridCFSBitmapScheduler.hpp>
#include <Concurrency/Spinlock.hpp>
#include <Concurrency/Thread.hpp>
#include <Concurrency/ThreadFlags.hpp>
#include <Concurrency/ThreadManager.hpp>
#include <Concurrency/ThreadPriority.hpp>
#include <Concurrency/ThreadState.hpp>
#include <Concurrency/Process.hpp>
#include <Concurrency/ProcessManager.hpp>
#include <Concurrency/ProcessState.hpp>
#include <Drivers/CPU/ICPUDriver.hpp>
#include <Drivers/Graphics/GraphicsDriverTypes.hpp>
#include <Drivers/Interrupts/IInterruptControllerDriver.hpp>
#include <Drivers/Timers/ITimerDriver.hpp>
#include <Interrupts/IInterruptContext.hpp>
#include <Interrupts/IInterruptControl.hpp>
#include <Interrupts/IInterruptManager.hpp>
#include <Interrupts/InterruptTypes.hpp>
#include <IPC/IPCPortService.hpp>
#include <KernelContext.hpp>
#include <KernelLog.hpp>
#include <Memory/AddressSpaceMap.hpp>
#include <Memory/AlignedHeapAllocationHeader.hpp>
#include <Memory/BuddyAllocator.hpp>
#include <Memory/BuddyConstants.hpp>
#include <Memory/BuddyFreeNode.hpp>
#include <Memory/FreeMemoryBlockListHead.hpp>
#include <Memory/FreeHeapBlock.hpp>
#include <Memory/HeapAllocator.hpp>
#include <Memory/HeapAllocatorConfiguration.hpp>
#include <Memory/HeapBlock.hpp>
#include <Memory/IAddressSpace.hpp>
#include <Memory/IAddressSpaceAllocator.hpp>
#include <Memory/IAddressTranslator.hpp>
#include <Memory/IMemoryAllocator.hpp>
#include <Memory/IMemoryMapper.hpp>
#include <Memory/MemoryMapping.hpp>
#include <Memory/MemoryMappingCache.hpp>
#include <Memory/MemoryMappingFlags.hpp>
#include <Memory/MemoryMappingOptions.hpp>
#include <Memory/MemoryMappingPermissions.hpp>
#include <Memory/MemoryPressure.hpp>
#include <Memory/MemoryRegion.hpp>
#include <Memory/MemoryRegionType.hpp>
#include <Memory/ObjectPool.hpp>
#include <Memory/SharedBuffer.hpp>
#include <Memory/SharedBufferRepository.hpp>
#include <Memory/SharedBufferResource.hpp>
#include <Memory/SharedBufferRights.hpp>
#include <Memory/Stack.hpp>
#include <Memory/StackManager.hpp>
#include <Resources/KernelResourceManager.hpp>
#include <UserMode/IUserModeManager.hpp>

#include "IA32InterruptConstants.hpp"
#include "IA32SystemCallHandler.hpp"
#include "IA32UserInterruptHandler.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  using namespace Kernel::Memory::ABI;

  using KernelMemoryInfo = Kernel::ABI::KernelMemoryInfo;
  using KernelMemoryTagStats = Kernel::ABI::KernelMemoryTagStats;
  using KernelMemoryPressureInfo = Kernel::ABI::KernelMemoryPressureInfo;
  using KernelPressureState = Kernel::ABI::KernelPressureState;

  IA32SystemCallHandler::IA32SystemCallHandler(
    KernelContext* context
  ) : _context(context) {}

  IA32InterruptContext* IA32SystemCallHandler::Handle(
    IA32InterruptContext& context
  ) {
    KernelOperation operation = static_cast<KernelOperation>(
      context.EAX
    );
    Process* process = _context->ProcessesManager->GetCurrent();
    ProcessID currentPID = process ? process->ID : 0;

    switch (operation) {
      // -----------------------------------------------------------------------
      // Process: Exit
      // -----------------------------------------------------------------------
      case KernelOperation::Process_Exit: {
        Int32 exitCode = static_cast<Int32>(context.EBX);

        // terminate the owning process
        Thread* current = _context->ThreadManager->GetCurrent();

        if (
          current &&
          current->OwnerProcess
        ) {
          _context->ProcessesManager->Terminate(
            current->OwnerProcess,
            exitCode
          );
        }

        // terminate the thread and switch to the next one
        return static_cast<IA32InterruptContext*>(
          _context->ThreadManager->Terminate(
            &context,
            exitCode
          )
        );
      }

      // -----------------------------------------------------------------------
      // Process: Spawn
      // -----------------------------------------------------------------------
      case KernelOperation::Process_Spawn: {
        const char* name = reinterpret_cast<const char*>(context.EBX);
        UIntPtr entryPoint =  context.ECX;
        const ProcessSpawnParameters* params
          = reinterpret_cast<const ProcessSpawnParameters*>(
              context.EDX
            );

        // validate user pointers before dereferencing
        if (
          !name ||
          !params ||
          !_isValidUserPointer(
            process,
            context.EDX,
            sizeof(ProcessSpawnParameters)
          ) ||
          !_isValidUserString(
            process,
            context.EBX
          )
        ) {
          context.EAX = static_cast<UInt32>(-1);

          break;
        }

        UIntPtr sourceBase = 0;
        Size sourceSizeInBytes = 0;
        UIntPtr targetBase = 0;
        Size segmentCount = 0;
        const ProcessSpawnSegment* segments = nullptr;
        Size argumentCount = 0;
        const char* argumentData = nullptr;
        Size argumentDataSize = 0;
        UInt8 streamCount = 0;
        SharedBufferID streamBufferIDs[3] = {};

        if (params) {
          sourceBase = params->SourceBase;
          sourceSizeInBytes = params->SizeInBytes;
          targetBase = params->TargetBase;
          segmentCount = params->SegmentCount;
          segments = params->Segments;
          argumentCount = params->ArgumentCount;
          argumentData = params->ArgumentData;
          argumentDataSize = params->ArgumentDataSize;
          streamCount = params->StreamCount;

          if (streamCount > 3) {
            streamCount = 3;
          }

          for (
            UInt8 streamIndex = 0;
            streamIndex < streamCount;
            streamIndex++
          ) {
            streamBufferIDs[streamIndex] = params->StreamBufferIDs[streamIndex];
          }

          // validate segments array pointer if provided
          if (
            segments &&
            segmentCount > 0 &&
            !_isValidUserPointer(
              process,
              reinterpret_cast<UIntPtr>(segments),
              segmentCount * sizeof(ProcessSpawnSegment)
            )
          ) {
            context.EAX = static_cast<UInt32>(-1);

            break;
          }

          // validate argument data pointer if provided
          if (
            argumentData &&
            argumentDataSize > 0 &&
            !_isValidUserPointer(
              process,
              reinterpret_cast<UIntPtr>(argumentData),
              argumentDataSize
            )
          ) {
            context.EAX = static_cast<UInt32>(-1);

            break;
          }
        }

        Process* parent = _context->ProcessesManager->GetCurrent();
        Process* child = _context->ProcessesManager->Spawn(
          name,
          entryPoint,
          parent,
          argumentCount,
          argumentData,
          argumentDataSize,
          sourceBase,
          sourceSizeInBytes,
          targetBase,
          segmentCount,
          segments,
          streamCount,
          streamBufferIDs
        );

        if (child) {
          _context->ThreadManager->Start(child->MainThread);

          context.EAX = child->ID;
        } else {
          context.EAX = static_cast<UInt32>(-1);
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Process: RequestPermission
      // -----------------------------------------------------------------------
      case KernelOperation::Process_RequestPermission: {
        ProcessPermissions requested = static_cast<ProcessPermissions>(
          context.EBX
        );

        context.EAX = 0; // not granted by default

        if (process) {
          // a process is trusted if the kernel process (PID 0) is anywhere
          // in its ancestor chain; the boot chain is:
          //   Kernel(0) -> StartupServer(1) -> RunServer(2) -> servers/drivers
          bool trusted = false;
          Process* ancestor = process;

          for (
            Size depth = 0;
            ancestor && depth < 8;
            ++depth
          ) {
            if (ancestor->ID == 0) {
              trusted = true;

              break;
            }

            ancestor = ancestor->Parent;
          }

          // non-trusted processes can only request permissions that their
          // parent already holds; this prevents privilege escalation
          bool parentHasPermissions
            = process->Parent
           && (process->Parent->Permissions & requested) == requested;

          if (
            trusted ||
            parentHasPermissions
          ) {
            process->Permissions
              = process->Permissions
              | requested;
            context.EAX = 1; // granted
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Process: GetID
      // -----------------------------------------------------------------------
      case KernelOperation::Process_GetID: {
        context.EAX = currentPID;

        break;
      }

      // -----------------------------------------------------------------------
      // Log: Write
      // -----------------------------------------------------------------------
      case KernelOperation::Log_Write: {
        const char* message = reinterpret_cast<const char*>(context.ECX);
        Size length = static_cast<Size>(context.EDX);

        if (
          message &&
          length > 0 &&
          _isValidUserPointer(
            process,
            context.ECX,
            length
          )
        ) {
          LogLevel level = static_cast<LogLevel>(context.EBX);

          _context->Log->Write(
            level,
            length,
            message
          );
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Log: SetLevel
      // -----------------------------------------------------------------------
      case KernelOperation::Log_SetLevel: {
        LogLevel level = static_cast<LogLevel>(context.EBX);

        _context->Log->SetMinLevel(level);

        break;
      }

      // -----------------------------------------------------------------------
      // Platform: GetDevices
      // -----------------------------------------------------------------------
      case KernelOperation::Drivers_GetDevices: {
        PointerList<IDriver*>& drivers = _context->Drivers;

        context.EAX = 0;
        context.EBX = 0;

        Size driverCount = drivers.GetCount();
        UInt32 devicesCount = static_cast<UInt32>(driverCount);

        if (
          process &&
          devicesCount > 0
        ) {
          Size driversSizeInBytes = driverCount * sizeof(Device);
          Size blockSize = _context->MemoryAllocator->GetBlockSize();

          // find an available block in the process address space to map the
          // device descriptors into
          Result<MemoryBlock> availableMemoryBlockResult
            = process->AddressSpaceMap.FindAvailableBlock(
              driversSizeInBytes,
              blockSize
            );

          if (availableMemoryBlockResult.Success) {
            UIntPtr devicesAddress = availableMemoryBlockResult.Data.Base;
            Size driversBlocksCount
              = AlignUp<Size>(
                  driversSizeInBytes,
                  blockSize
                )
              / blockSize;
            MemoryMappingFlags memoryMappingFlags {
              MemoryMappingPermissions::Read
                | MemoryMappingPermissions::User,
              MemoryMappingCache::Default,
              MemoryMappingOptions::None
            };
            bool memoryMappingFailed = false;

            // do the mapping
            for (
              Size driversBlocksIndex = 0;
              driversBlocksIndex < driversBlocksCount;
              driversBlocksIndex++
            ) {
              MemoryBlock kernelMemoryBlock
                = _context->MemoryAllocator->Allocate(
                    blockSize,
                    MemoryBlockTag::DeviceInfo
                  );

              if (kernelMemoryBlock.Base == 0) {
                for (
                  Size driverIndex = 0;
                  driverIndex < driversBlocksIndex;
                  driverIndex++
                ) {
                  MemoryBlock freed = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      devicesAddress + driverIndex * blockSize,
                      blockSize
                    }
                  );

                  if (freed.SizeInBytes > 0) {
                    _context->MemoryAllocator->Free(freed);
                  }
                }

                memoryMappingFailed = true;

                break;
              } else {
                _context->MemoryMapper->Map(
                  *process->AddressSpace,
                  MemoryBlock {
                    devicesAddress + driversBlocksIndex * blockSize,
                    blockSize
                  },
                  kernelMemoryBlock,
                  memoryMappingFlags
                );
              }
            }

            if (memoryMappingFailed) {
              _context->Log->Error(
                "Platform_GetDevices failed to map memory for device "
                "descriptors in PID %u",
                currentPID
              );

              break;
            } else {
              process->AddressSpaceMap.Insert(
                MemoryMapping {
                  process->AddressSpace,
                  MemoryBlock {
                    devicesAddress,
                    driversBlocksCount * blockSize
                  },
                  MemoryBlock {},
                  memoryMappingFlags
                }
              );

              process->TotalBlockCount += driversBlocksCount;

              Device* devices = reinterpret_cast<Device*>(devicesAddress);

              // copy device descriptors into user space
              for (
                Size deviceIndex = 0;
                deviceIndex < driverCount;
                deviceIndex++
              ) {
                devices[deviceIndex] = drivers[deviceIndex]->GetDevice();
              }

              context.EAX = devicesAddress;
              context.EBX = devicesCount;
            }
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Platform: InvokeDriver
      // -----------------------------------------------------------------------
      case KernelOperation::Drivers_Invoke: {
        DeviceID deviceID = static_cast<DeviceID>(context.EBX);
        UInt32 driverOperation = context.ECX;
        void* driverOperationPayload = reinterpret_cast<void*>(context.EDX);

        context.EAX = static_cast<UInt32>(-1);

        IDriver* driver = nullptr;

        for (
          Size driverIndex = 0;
          driverIndex < _context->Drivers.GetCount();
          driverIndex++
        ) {
          IDriver* candidateDriver = _context->Drivers[driverIndex];

          if (candidateDriver->GetDeviceID() == deviceID) {
            driver = candidateDriver;

            break;
          }
        }

        // validate payload pointer if non-null; full size validation
        // would require per-operation size declarations, so for now just
        // verify the base address is in a valid user mapping
        if (
          driverOperationPayload &&
          _isValidUserPointer(
            process,
            reinterpret_cast<UIntPtr>(driverOperationPayload),
            1
          ) &&
          driver
        ) {
          context.EAX = driver->Invoke(
            driverOperation,
            driverOperationPayload
          );
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: Allocate
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_Allocate: {
        Size sizeInBytes = static_cast<Size>(context.EBX);

        context.EAX = 0; // null on failure

        if (
          sizeInBytes > 0 &&
          process
        ) {
          Size blockSize = _context->MemoryAllocator->GetBlockSize();
          Result<MemoryBlock> findResult
            = process->AddressSpaceMap.FindAvailableBlock(
              sizeInBytes,
              blockSize
            );

          // if we found an available process block, allocate kernel blocks,
          // map them to it, and return the process address to the caller
          if (findResult.Success) {
            UIntPtr address = findResult.Data.Base;

            // calculate how many kernel blocks we need
            Size blockCount
              = AlignUp<Size>(
                  sizeInBytes,
                  blockSize
                )
              / blockSize;

            MemoryMappingFlags mapFlags {
              MemoryMappingPermissions::Read
                | MemoryMappingPermissions::Write
                | MemoryMappingPermissions::User,
              MemoryMappingCache::Default,
              MemoryMappingOptions::None
            };

            // allocate and map kernel blocks to the found process address
            bool allocationFailed = false;

            for (
              Size blockIndex = 0;
              blockIndex < blockCount;
              blockIndex++
            ) {
              MemoryBlock blockToMap = _context->MemoryAllocator->Allocate(
                  blockSize,
                  MemoryBlockTag::ProcessHeap
                );

              if (blockToMap.Base == 0) {
                // allocation failed, unmap any blocks we already mapped
                for (
                  Size mappedIndex = 0;
                  mappedIndex < blockIndex;
                  mappedIndex++
                ) {
                  MemoryBlock freed
                    = _context->MemoryMapper->Unmap(
                        *process->AddressSpace,
                        MemoryBlock {
                          address + mappedIndex * blockSize,
                          blockSize
                        }
                      );

                  if (freed.SizeInBytes > 0) {
                    _context->MemoryAllocator->Free(freed);
                  }
                }

                allocationFailed = true;

                break;
              } else {
                MemoryBlock processBlock {
                  address + blockIndex * blockSize,
                  blockSize
                };
                MemoryBlock kernelBlock = _context->MemoryMapper->Map(
                  *process->AddressSpace,
                  processBlock,
                  blockToMap,
                  mapFlags
                );

                if (kernelBlock.SizeInBytes == 0) {
                  // mapping failed, free the allocated block ...
                  _context->MemoryAllocator->Free(blockToMap);

                  // ... and unmap any blocks we already mapped
                  for (
                    Size mappedIndex = 0;
                    mappedIndex < blockIndex;
                    mappedIndex++
                  ) {
                    MemoryBlock freed
                      = _context->MemoryMapper->Unmap(
                          *process->AddressSpace,
                          MemoryBlock {
                            address + mappedIndex * blockSize,
                            blockSize
                          }
                        );

                    if (freed.SizeInBytes > 0) {
                      _context->MemoryAllocator->Free(freed);
                    }
                  }

                  allocationFailed = true;

                  break;
                }
              }

              if (allocationFailed && _context->MemoryPressureMonitor) {
                _context->MemoryPressureMonitor->Reclaim(MemoryPressure::Elevated);

                allocationFailed = false;

                for (
                  Size blockIndex = 0;
                  blockIndex < blockCount;
                  blockIndex++
                ) {
                  MemoryBlock blockToMap
                    = _context->MemoryAllocator->Allocate(
                        blockSize, MemoryBlockTag::ProcessHeap
                      );

                  if (blockToMap.Base > 0) {
                    MemoryBlock processBlock {
                      address + blockIndex * blockSize,
                      blockSize
                    };
                    _context->MemoryMapper->Map(
                      *process->AddressSpace,
                      processBlock,
                      blockToMap,
                      mapFlags
                    );
                  } else {
                    for (
                      Size mappedIndex = 0;
                      mappedIndex < blockIndex;
                      mappedIndex++
                    ) {
                      MemoryBlock freed
                        = _context->MemoryMapper->Unmap(
                            *process->AddressSpace,
                            MemoryBlock {
                              address + mappedIndex * blockSize,
                              blockSize
                            }
                          );

                      if (freed.SizeInBytes > 0) {
                        _context->MemoryAllocator->Free(freed);
                      }
                    }

                    allocationFailed = true;

                    break;
                  }
                }
              }

              if (allocationFailed) {
                return &context;
              } else {
                // record a single mapping covering the full allocation so
                // Memory_Free can find and release all pages in one operation
                process->AddressSpaceMap.Insert(
                  MemoryMapping {
                    process->AddressSpace,
                    MemoryBlock { address, blockCount * blockSize },
                    MemoryBlock {},
                    mapFlags,
                    MemoryRegionType::Heap
                  }
                );

                process->HeapBlockCount += blockCount;
                process->TotalBlockCount += blockCount;

                // return the allocated process address to the caller
                context.EAX = findResult.Data.Base;
              }
            }
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: AllocateRange
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_AllocateRange: {
        Size sizeInBytes = static_cast<Size>(context.EBX);

        context.EAX = 0; // null on failure

        if (
          sizeInBytes > 0 &&
          process
        ) {
          Size blockSize = _context->MemoryAllocator->GetBlockSize();
          Size totalSize = AlignUp<Size>(sizeInBytes, blockSize);

          Result<MemoryBlock> findResult
            = process->AddressSpaceMap.FindAvailableBlock(
                totalSize,
                blockSize
              );

          if (findResult.Success) {
            UIntPtr address = findResult.Data.Base;

            // record a lazy VMA, no kernel pages are allocated here;
            // they will be faulted in on first access by the demand pager
            process->AddressSpaceMap.Insert(
              MemoryMapping {
                process->AddressSpace,
                MemoryBlock { address, totalSize },
                MemoryBlock {},
                MemoryMappingFlags {
                  MemoryMappingPermissions::Read |
                  MemoryMappingPermissions::Write |
                  MemoryMappingPermissions::User,
                  MemoryMappingCache::Default,
                  MemoryMappingOptions::Lazy
                },
                MemoryRegionType::Heap
              }
            );

            context.EAX = address;
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: Free
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_Free: {
        UIntPtr address = context.EBX;

        if (process) {
          Result<MemoryMapping> findResult
            = process->AddressSpaceMap.FindContainingBlock(address);

          if (findResult.Success) {
            MemoryMapping mapping = findResult.Data;
            Size blockSize = _context->MemoryAllocator->GetBlockSize();
            Size blockCount
              = AlignUp<Size>(
                  mapping.ProcessBlock.SizeInBytes,
                  blockSize
                )
              / blockSize;

            // unmap each block and free the backing kernel memory
            for (
              Size blockIndex = 0;
              blockIndex < blockCount;
              blockIndex++
            ) {
              MemoryBlock kernelBlock
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      mapping.ProcessBlock.Base + blockIndex * blockSize,
                      blockSize
                    }
                  );

              if (kernelBlock.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(kernelBlock);
              }
            }

            process->AddressSpaceMap.Remove(mapping);

            if (process->HeapBlockCount >= blockCount) {
              process->HeapBlockCount -= blockCount;
            } else {
              process->HeapBlockCount = 0;
            }

            if (process->TotalBlockCount >= blockCount) {
              process->TotalBlockCount -= blockCount;
            } else {
              process->TotalBlockCount = 0;
            }
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: GetInfo
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_GetInfo: {
        auto* info = reinterpret_cast<KernelMemoryInfo*>(context.EBX);

        context.EAX = 0;

        if (!info) break;

        // validate user buffer
        if (!_isValidUserPointer(
          process, context.EBX, sizeof(KernelMemoryInfo)
        )) break;

        info->TotalBytes = _context->MemoryAllocator->GetManagedBytes();
        info->BlockSize = _context->MemoryAllocator->GetBlockSize();
        info->BlockCount = _context->MemoryAllocator->GetBlockCount();
        info->UsedBlockCount = _context->MemoryAllocator->GetUsedBlockCount();
        info->UsedBytes = info->UsedBlockCount * info->BlockSize;
        info->FreeBytes = info->TotalBytes - info->UsedBytes;

        info->InitialImageBytes
          = _context->MemoryAllocator->GetInitialImageBytes();
        info->KernelReservedBytes
          = _context->MemoryAllocator->GetReservedBytes()
          - _context->MemoryAllocator->GetInitialImageBytes();
        info->KernelHeapBytes = _context->HeapAllocator->GetMappedBytes();
        info->UserProcessBytes
          = _context->ProcessesManager->GetTotalUserPages()
          * info->BlockSize;

        context.EAX = 1;

        break;
      }

      case KernelOperation::Memory_GetPressureInfo: {
        auto* info = reinterpret_cast<KernelMemoryPressureInfo*>(
          context.EBX
        );

        context.EAX = 0;

        if (!info || !_context->MemoryPressureMonitor) break;

        // validate user buffer
        if (!_isValidUserPointer(
          process, context.EBX,
          sizeof(KernelMemoryPressureInfo)
        )) break;

        MemoryPressureDiagnostics diag;
        _context->MemoryPressureMonitor->UpdateState();
        _context->MemoryPressureMonitor->GetDiagnostics(&diag);

        info->TotalBlocks = diag.TotalBlocks;
        info->FreeBlocks = diag.FreeBlocks;
        info->UsedBlocks = diag.UsedBlocks;
        info->KernelHeapBytes = diag.KernelHeapBytes;
        info->SharedBufferCount = diag.SharedBufferCount;
        info->PoolIPCMessageBytes = diag.PoolIPCMessageBytes;
        info->PoolIPCPortBytes = diag.PoolIPCPortBytes;
        info->PoolSharedBufferBytes = diag.PoolSharedBufferBytes;
        info->State = static_cast<KernelPressureState>(diag.State);

        context.EAX = 1;

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: GetTagStats
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_GetTagStats: {
        auto* stats = reinterpret_cast<KernelMemoryTagStats*>(
          context.EBX
        );

        context.EAX = 0;

        if (!stats) break;

        if (!_isValidUserPointer(
          process, context.EBX, sizeof(KernelMemoryTagStats)
        )) break;

        stats->BlockSize = _context->MemoryAllocator->GetBlockSize();

        for (
          UInt8 tagIndex = 0;
          tagIndex < static_cast<UInt8>(MemoryBlockTag::Count);
          tagIndex++
        ) {
          auto tag = static_cast<MemoryBlockTag>(tagIndex);

          stats->BlockCounts[tagIndex]
            = _context->MemoryAllocator->GetUsedBlockCountByTag(tag);

          const char* name = MemoryBlockTagName(tag);
          Size nameLength = 0;

          while (name[nameLength] && nameLength < 19) {
            stats->TagNames[tagIndex][nameLength] = name[nameLength];
            nameLength++;
          }

          stats->TagNames[tagIndex][nameLength] = '\0';
        }

        context.EAX = 1;

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: FreeInitialImage
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_FreeInitialImage: {
        _context->MemoryAllocator->FreeInitialImage();

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: CreateShared
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_CreateShared: {
        Size sizeInBytes = static_cast<Size>(context.EBX);

        context.EAX = 0; // null on failure

        if (sizeInBytes == 0 || !process) break;

        Size blockSize  = _context->MemoryAllocator->GetBlockSize();
        Size blockCount = AlignUp<Size>(sizeInBytes, blockSize) / blockSize;

        // allocate kernel blocks (no VAS mapping, caller uses AttachShared)
        auto* blocks          = new MemoryBlock[blockCount];
        bool allocationFailed = false;

        for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
          MemoryBlock phys
            = _context->MemoryAllocator->Allocate(
                blockSize, MemoryBlockTag::SharedBuffer
              );

          if (phys.Base == 0) {
            // rollback already-allocated blocks
            for (Size j = 0; j < blockIndex; j++) {
              _context->MemoryAllocator->Free(blocks[j]);
            }

            allocationFailed = true;
            break;
          }

          blocks[blockIndex] = phys;
        }

        if (allocationFailed && _context->MemoryPressureMonitor) {
          _context->MemoryPressureMonitor->Reclaim(MemoryPressure::Elevated);

          allocationFailed = false;

          for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
            MemoryBlock phys
              = _context->MemoryAllocator->Allocate(
                  blockSize, MemoryBlockTag::SharedBuffer
                );

            if (phys.Base == 0) {
              for (Size j = 0; j < blockIndex; j++) {
                _context->MemoryAllocator->Free(blocks[j]);
              }

              allocationFailed = true;
              break;
            }

            blocks[blockIndex] = phys;
          }
        }

        if (allocationFailed) {
          KLOG_TRACE(
            "CreateShared REJECT PID %u: kernel alloc failed",
            process->ID
          );

          delete[] blocks;
          break;
        }

        // register the shared buffer (AttachCount starts at 0)
        SharedBuffer* buf
          = _context->SharedBufferRepository->Create(blocks, blockCount, sizeInBytes);

        // track ownership so the buffer is cleaned up on process death
        auto resourceResult
          = _context->KernelResourceManager->Create<SharedBufferResource>(
              KernelResourceType::SharedBuffer,
              buf->ID,
              SharedBufferRights::Owner,
              process->ID
            );

        if (!resourceResult.Success) {
          _context->SharedBufferRepository->Remove(buf->ID);
          break;
        }

        KLOG_TRACE(
          "CreateShared OK PID %u BufferID %u, %u blocks, %u total",
          process->ID,
          buf->ID,
          blockCount,
          process->TotalBlockCount
        );

        context.EAX = buf->ID;

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: AttachShared
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_AttachShared: {
        SharedBufferID bufferID = static_cast<SharedBufferID>(context.EBX);

        context.EAX = 0; // null on failure

        if (!process) break;

        SharedBuffer* buf = _context->SharedBufferRepository->Find(bufferID);

        if (!buf) break;

        Size blockSize = _context->MemoryAllocator->GetBlockSize();
        Size totalSize = buf->BlockCount * blockSize;

        Result<MemoryBlock> findResult
          = process->AddressSpaceMap.FindAvailableBlock(
              totalSize,
              blockSize
            );

        if (!findResult.Success) break;

        UIntPtr address = findResult.Data.Base;

        MemoryMappingFlags mapFlags {
          MemoryMappingPermissions::Read |
          MemoryMappingPermissions::Write |
          MemoryMappingPermissions::User,
          buf->CacheMode,
          MemoryMappingOptions::None
        };

        bool attachFailed = false;

        for (Size i = 0; i < buf->BlockCount; i++) {
          // increment the kernel page ref-count before mapping
          if (!buf->Pinned) {
            _context->MemoryAllocator->Retain(buf->Blocks[i]);
          }

          MemoryBlock vb { address + i * blockSize, blockSize };
          MemoryBlock mapped = _context->MemoryMapper->Map(
            *process->AddressSpace,
            vb,
            buf->Blocks[i],
            mapFlags
          );

          if (mapped.SizeInBytes == 0) {
            // Map failed: undo the Retain for this block
            if (!buf->Pinned) {
              _context->MemoryAllocator->Free(buf->Blocks[i]);
            }

            // rollback already-mapped blocks (Unmap + dec their Retain)
            for (Size j = 0; j < i; j++) {
              _context->MemoryMapper->Unmap(
                *process->AddressSpace,
                MemoryBlock { address + j * blockSize, blockSize }
              );
              if (!buf->Pinned) {
                _context->MemoryAllocator->Free(buf->Blocks[j]);
              }
            }

            attachFailed = true;
            break;
          }
        }

        if (attachFailed) break;

        buf->AttachCount++;

        // track the attachment so the buffer is detached on process death
        _context->KernelResourceManager->Create<SharedBufferResource>(
          KernelResourceType::SharedBuffer,
          buf->ID,
          SharedBufferRights::Attach,
          process->ID
        );

        process->AddressSpaceMap.Insert(
          MemoryMapping {
            process->AddressSpace,
            MemoryBlock { address, totalSize },
            MemoryBlock {},
            mapFlags,
            MemoryRegionType::Shared,
            bufferID
          }
        );

        process->TotalBlockCount += buf->BlockCount;

        context.EAX = address;

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: DetachShared
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_DetachShared: {
        UIntPtr address = context.EBX;

        if (!process) break;

        Result<MemoryMapping> findResult
          = process->AddressSpaceMap.FindContainingBlock(address);

        if (!findResult.Success) break;

        MemoryMapping mapping = findResult.Data;

        if (mapping.RegionType != MemoryRegionType::Shared) break;

        Size blockSize  = _context->MemoryAllocator->GetBlockSize();
        Size blockCount = mapping.ProcessBlock.SizeInBytes / blockSize;

        SharedBuffer* buf
          = _context->SharedBufferRepository->Find(mapping.SharedBufferID);

        // unmap each process block and decrement the kernel ref-count
        for (Size i = 0; i < blockCount; i++) {
          MemoryBlock vb {
            mapping.ProcessBlock.Base + i * blockSize,
            blockSize
          };
          MemoryBlock phys = _context->MemoryMapper->Unmap(
            *process->AddressSpace,
            vb
          );

          if (phys.SizeInBytes > 0 && !(buf && buf->Pinned)) {
            _context->MemoryAllocator->Free(phys);
          }
        }

        process->AddressSpaceMap.Remove(mapping);

        if (process->TotalBlockCount >= blockCount) {
          process->TotalBlockCount -= blockCount;
        } else {
          process->TotalBlockCount = 0;
        }

        // remove the Attach resource for this process and buffer
        {
          ResourceID attachResourceID = 0;

          _context->KernelResourceManager->ForEachMatching<SharedBufferResource>(
            KernelResourceType::SharedBuffer,
            mapping.SharedBufferID,
            [&](SharedBufferResource* resource) -> bool {
              if (
                resource->OwnerPID == process->ID
                && ::Quantum::Core::Enum::HasAnyFlag(
                     resource->Rights,
                     SharedBufferRights::Attach
                   )
              ) {
                attachResourceID = resource->ID;
                return false; // stop iteration
              }

              return true;
            }
          );

          if (attachResourceID != 0) {
            auto removeResult
              = _context->KernelResourceManager->Remove(attachResourceID);

            if (removeResult.Success) {
              removeResult.Data->DecrementRefCount();
            }
          }
        }

        if (buf) {
          if (buf->AttachCount > 0) buf->AttachCount--;

          if (buf->AttachCount == 0 && !buf->Pinned) {
            // Balance the initial Allocate() refcount. Each page starts at
            // refcount=1 from Allocate; every AttachShared/DetachShared pair
            // covers one Retain/Free, but the original refcount=1 is never
            // decremented by the registry cleanup. Without this Free loop,
            // every destroyed shared buffer permanently leaks its kernel
            // pages (they stay in the bitmap as "used" with refcount=1).
            for (Size blockIdx = 0; blockIdx < buf->BlockCount; ++blockIdx) {
              _context->MemoryAllocator->Free(buf->Blocks[blockIdx]);
            }

            _context->SharedBufferRepository->Remove(mapping.SharedBufferID);
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: VirtualToPhysical
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_VirtualToPhysical: {
        UIntPtr processAddress = context.EBX;

        UIntPtr kernelAddress
          = _context->AddressTranslator->VirtualToPhysical(processAddress);

        context.EAX = static_cast<UInt32>(kernelAddress);

        break;
      }

      // -----------------------------------------------------------------------
      // Memory: AllocateDMA
      // -----------------------------------------------------------------------
      case KernelOperation::Memory_AllocateDMA: {
        Size sizeInBytes = static_cast<Size>(context.EBX);
        UInt32 kernelLimit = context.ECX;

        context.EAX = 0;

        if (sizeInBytes > 0 && kernelLimit > 0 && process) {
          Size blockSize = _context->MemoryAllocator->GetBlockSize();
          Result<MemoryBlock> findResult
            = process->AddressSpaceMap.FindAvailableBlock(
              sizeInBytes,
              blockSize
            );

          if (findResult.Success) {
            UIntPtr address = findResult.Data.Base;

            Size blockCount
              = AlignUp<Size>(sizeInBytes, blockSize) / blockSize;

            MemoryMappingFlags mapFlags {
              MemoryMappingPermissions::Read |
              MemoryMappingPermissions::Write |
              MemoryMappingPermissions::User,
              MemoryMappingCache::Default,
              MemoryMappingOptions::None
            };

            bool allocationFailed = false;

            for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
              MemoryBlock blockToMap
                = _context->MemoryAllocator->AllocateBelow(kernelLimit);

              if (blockToMap.Base == 0) {
                for (
                  Size mappedIndex = 0;
                  mappedIndex < blockIndex;
                  mappedIndex++
                ) {
                  MemoryBlock freed
                    = _context->MemoryMapper->Unmap(
                        *process->AddressSpace,
                        MemoryBlock {
                          address + mappedIndex * blockSize,
                          blockSize
                        }
                      );

                  if (freed.SizeInBytes > 0) {
                    _context->MemoryAllocator->Free(freed);
                  }
                }

                allocationFailed = true;

                break;
              }

              MemoryBlock processBlock {
                address + blockIndex * blockSize,
                blockSize
              };
              MemoryBlock kernelBlock = _context->MemoryMapper->Map(
                *process->AddressSpace,
                processBlock,
                blockToMap,
                mapFlags
              );

              if (kernelBlock.SizeInBytes == 0) {
                _context->MemoryAllocator->Free(blockToMap);

                for (
                  Size mappedIndex = 0;
                  mappedIndex < blockIndex;
                  mappedIndex++
                ) {
                  MemoryBlock freed
                    = _context->MemoryMapper->Unmap(
                        *process->AddressSpace,
                        MemoryBlock {
                          address + mappedIndex * blockSize,
                          blockSize
                        }
                      );

                  if (freed.SizeInBytes > 0) {
                    _context->MemoryAllocator->Free(freed);
                  }
                }

                allocationFailed = true;

                break;
              }
            }

            if (!allocationFailed) {
              process->AddressSpaceMap.Insert(
                MemoryMapping {
                  process->AddressSpace,
                  MemoryBlock { address, blockCount * blockSize },
                  MemoryBlock {},
                  mapFlags,
                  MemoryRegionType::Heap
                }
              );

              process->HeapBlockCount += blockCount;
              process->TotalBlockCount += blockCount;

              context.EAX = findResult.Data.Base;
            }
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Process: GetList
      // -----------------------------------------------------------------------
      case KernelOperation::Process_GetList: {
        auto* buffer = reinterpret_cast<ProcessInfo*>(
          context.EBX
        );
        Size maxCount = static_cast<Size>(context.ECX);

        context.EAX = 0;

        if (!buffer || maxCount == 0) break;

        // validate that the entire output buffer is in mapped user memory
        if (!_isValidUserPointer(
          process,
          reinterpret_cast<UIntPtr>(buffer),
          maxCount * sizeof(ProcessInfo)
        )) break;

        context.EAX = static_cast<UInt32>(
          _context->ProcessesManager->EnumerateProcesses(buffer, maxCount)
        );

        break;
      }

      // -----------------------------------------------------------------------
      // Process: Wait
      // -----------------------------------------------------------------------
      case KernelOperation::Process_Wait: {
        ProcessID targetPID = static_cast<ProcessID>(context.EBX);
        Process* target = _context->ProcessesManager->GetByID(targetPID);

        if (!target) {
          context.EAX = static_cast<UInt32>(-1);

          break;
        }

        // already dead, return exit code immediately
        if (target->State == ProcessState::Zombie) {
          context.EAX = static_cast<UInt32>(target->ExitCode);

          break;
        }

        // add current thread to the target's wait queue and sleep
        auto* waitNode = new LinkedNode<Thread*>(
          _context->ThreadManager->GetCurrent()
        );

        target->WaitQueue.Append(waitNode);

        _context->ThreadManager->Sleep(_context->ThreadManager->GetCurrent(), 0);

        // after waking: process should be zombie
        context.EAX = static_cast<UInt32>(target->ExitCode);

        break;
      }

      // -----------------------------------------------------------------------
      // Process: IsAlive
      // -----------------------------------------------------------------------
      case KernelOperation::Process_IsAlive: {
        ProcessID targetPID = static_cast<ProcessID>(context.EBX);
        Process* target
          = _context->ProcessesManager->GetByID(targetPID);

        if (
          !target ||
          target->State == ProcessState::Zombie
        ) {
          context.EAX = 0;
        } else {
          context.EAX = 1;
        }

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: Open
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_Open: {
        IPCPortID portID = static_cast<IPCPortID>(context.EBX);
        IPCPortRights rights = static_cast<IPCPortRights>(context.ECX);

        context.EAX = static_cast<UInt32>(-1); // failure

        if (!process) break;

        // auto-assign a unique port ID when the caller passes -1
        if (
          portID == static_cast<IPCPortID>(-1) &&
          Enum::HasAnyFlag(rights, IPCPortRights::Manage)
        ) {
          portID = _context->IPCPortRepository->AllocateID();
          context.EBX = static_cast<UInt32>(portID);
        }

        Result<IPCPort*> portResult = _context->IPCPortRepository->FindByID(portID);

        // if the port doesn't exist and Manage right is requested,
        // auto-create and register it
        if (
          !portResult.Success &&
          Enum::HasAnyFlag(rights, IPCPortRights::Manage)
        ) {
          IPCPort* port = new (_context->IPCPortPool->Allocate()) IPCPort();

          port->ID = portID;
          port->OwnerProcess = process;

          _context->IPCPortRepository->Save(port);

          portResult = Result<IPCPort*>(true, port);
        }

        if (!portResult.Success) break;

        // exclusivity check: if requesting Receive or Manage, ensure no
        // existing resource for this port already holds those rights
        if (
          Enum::HasAnyFlag(
            rights,
            IPCPortRights::Receive | IPCPortRights::Manage
          )
        ) {
          bool alreadyExclusive = false;

          _context->KernelResourceManager->ForEachMatching<IPCPortResource>(
            KernelResourceType::IPCPort,
            portID,
            [&alreadyExclusive](IPCPortResource* existing) {
              if (
                Enum::HasAnyFlag(
                  existing->Rights,
                  IPCPortRights::Receive | IPCPortRights::Manage
                )
              ) {
                alreadyExclusive = true;

                return false;
              }

              return true;
            }
          );

          if (alreadyExclusive) break;
        }

        Result<IPCPortResource*> createResult
          = _context->KernelResourceManager->Create<IPCPortResource>(
              KernelResourceType::IPCPort,
              portID,
              rights,
              currentPID
            );

        if (createResult.Success) {
          _context->Log->Debug(
            "Allocated port resource %u (port ID %u) with rights "
            "%p to PID %u",
            createResult.Data->ID,
            createResult.Data->ObjectID,
            Enum::ToBase(createResult.Data->Rights),
            currentPID
          );

          context.EAX = createResult.Data->ID;
        }

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: Close
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_Close: {
        IPCPortResourceID handle = static_cast<IPCPortResourceID>(context.EBX);

        Result<IPCPortResource*> getResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResult.Success) break;

        IPCPortResource* resource = getResult.Data;

        // access check: process can only close its own resource handles
        if (resource->OwnerPID != currentPID) break;

        IPCPortID portID = resource->ObjectID;
        bool isManager = Enum::HasAnyFlag(resource->Rights, IPCPortRights::Manage);

        Result<KernelResourceBase*> removeResult = _context->KernelResourceManager->Remove(handle);

        if (!removeResult.Success) break;

        _context->Log->Debug(
          "Freed port resource %u (port %u, rights %u, ref. count %u, "
          "address %p)",
          handle,
          portID,
          static_cast<UInt32>(resource->Rights),
          resource->GetRefCount(),
          reinterpret_cast<UIntPtr>(resource)
        );

        if (resource->DecrementRefCount() == 0) {
          _context->Log->Debug(
            "Deleted port resource at %p",
            reinterpret_cast<UIntPtr>(resource)
          );

          if (isManager) {
            _context->IPCPortRepository->RemoveByID(portID);
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: Send
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_Send: {
        IPCPortResourceID handle = static_cast<IPCPortResourceID>(context.EBX);
        void* userPayload = reinterpret_cast<void*>(context.ECX);
        Size payloadSize = static_cast<Size>(context.EDX);

        Result<IPCPortResource*> getResourceResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResourceResult.Success) break;

        // verify sender has Send right
        if (!Enum::HasAnyFlag(getResourceResult.Data->Rights, IPCPortRights::Send)) break;

        Result<IPCPort*> portResult
          = _context->IPCPortRepository->FindByID(getResourceResult.Data->ObjectID);

        if (!portResult.Success) break;

        // copy the payload from user space into a kernel heap buffer
        void* kernelPayload = nullptr;

        if (userPayload && payloadSize > 0) {
          if (!_isValidUserPointer(
            process,
            reinterpret_cast<UIntPtr>(userPayload),
            payloadSize
          )) break;

          kernelPayload = _context->HeapAllocator->Allocate(payloadSize);

          if (!kernelPayload) break;

          Byte::Copy(kernelPayload, userPayload, payloadSize);
        }

        // create the IPC message from the pool
        void* messageSlot = _context->IPCMessagePool->Allocate();

        if (!messageSlot) {
          _context->Log->Warning(
            "IPC message pool exhausted, dropping message"
          );

          if (kernelPayload) {
            _context->HeapAllocator->Free(kernelPayload);
          }

          context.EAX = static_cast<UInt32>(-1);

          break;
        }

        auto* message = new (messageSlot) IPCMessage();

        message->SendingProcessID = process ? process->ID : 0;
        message->Payload = kernelPayload;
        message->PayloadSizeInBytes = payloadSize;

        // send the message (blocks if queue is full)
        _context->IPCPortService->Send(portResult.Data, message);

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: SendShared (zero-copy shared-buffer descriptor)
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_SendShared: {
        IPCPortResourceID handle
          = static_cast<IPCPortResourceID>(context.EBX);
        auto* userDescriptor
          = reinterpret_cast<IPCSharedDescriptor*>(context.ECX);

        context.EAX = 0; // failure

        if (!process) break;
        if (!userDescriptor) break;

        if (!_isValidUserPointer(
          process,
          reinterpret_cast<UIntPtr>(userDescriptor),
          sizeof(IPCSharedDescriptor)
        )) break;

        // validate handle and Send right
        Result<IPCPortResource*> getResourceResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResourceResult.Success) break;

        if (!Enum::HasAnyFlag(
          getResourceResult.Data->Rights, IPCPortRights::Send
        )) break;

        Result<IPCPort*> portResult
          = _context->IPCPortRepository->FindByID(
              getResourceResult.Data->ObjectID
            );

        if (!portResult.Success) break;

        // copy descriptor from user space
        IPCSharedDescriptor descriptor;
        Byte::Copy(&descriptor, userDescriptor, sizeof(descriptor));

        // validate the shared buffer exists and range is in-bounds
        SharedBuffer* buf
          = _context->SharedBufferRepository->Find(descriptor.BufferID);

        if (!buf) break;

        if (descriptor.Offset + descriptor.Size < descriptor.Offset) break; // overflow
        if (descriptor.Offset + descriptor.Size > buf->SizeInBytes) break;

        // validate that the sending process has the buffer attached
        bool senderHasAttach = false;

        _context->KernelResourceManager->ForEachMatching<SharedBufferResource>(
          KernelResourceType::SharedBuffer,
          descriptor.BufferID,
          [&](SharedBufferResource* res) -> bool {
            if (res->OwnerPID == process->ID &&
                Enum::HasAnyFlag(
                  res->Rights, SharedBufferRights::Attach
                )) {
              senderHasAttach = true;
              return false; // stop iteration
            }
            return true; // continue
          }
        );

        if (!senderHasAttach) break;

        // allocate IPC message from pool
        void* messageSlot = _context->IPCMessagePool->Allocate();

        if (!messageSlot) {
          _context->Log->Warning(
            "IPC message pool exhausted, dropping shared message"
          );

          context.EAX = static_cast<UInt32>(-1);

          break;
        }

        auto* message = new (messageSlot) IPCMessage();

        message->SendingProcessID = process->ID;
        message->Payload = nullptr;
        message->PayloadSizeInBytes = 0;
        message->Type = IPCMessageType::Shared;
        message->SharedDescriptor = descriptor;

        _context->IPCPortService->Send(portResult.Data, message);

        context.EAX = 1; // success

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: Receive
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_Receive: {
        IPCPortResourceID handle = static_cast<IPCPortResourceID>(context.EBX);

        context.EAX = 0; // null on failure

        if (!process) break;

        Result<IPCPortResource*> getResourceResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResourceResult.Success) break;

        // verify receiver has Receive right
        if (!Enum::HasAnyFlag(getResourceResult.Data->Rights, IPCPortRights::Receive)) {
          break;
        }

        Result<IPCPort*> portResult
          = _context->IPCPortRepository->FindByID(getResourceResult.Data->ObjectID);

        if (!portResult.Success) break;

        // receive the message (blocks until one arrives)
        IPCMessage* message = _context->IPCPortService->Receive(portResult.Data);

        if (!message) break;

        // allocate user-space memory for the IPCMessage struct + payload
        Size totalSize = sizeof(IPCMessage) + message->PayloadSizeInBytes;
        Size blockSize = _context->MemoryAllocator->GetBlockSize();

        Result<MemoryBlock> userFindResult
          = process->AddressSpaceMap.FindAvailableBlock(
            totalSize,
            blockSize
          );

        if (!userFindResult.Success) {
          // can't allocate user memory, free kernel copies
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        UIntPtr userBase = userFindResult.Data.Base;
        Size blockCount = AlignUp<Size>(totalSize, blockSize) / blockSize;
        bool mapFailed = false;
        MemoryMappingFlags mapFlags {
          MemoryMappingPermissions::Read |
          MemoryMappingPermissions::Write |
          MemoryMappingPermissions::User,
          MemoryMappingCache::Default,
          MemoryMappingOptions::None
        };

        for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
          MemoryBlock blockToMap
            = _context->MemoryAllocator->Allocate(
                blockSize, MemoryBlockTag::IPCReceiveBuffer
              );

          if (blockToMap.Base == 0) {
            // rollback already-mapped blocks and free their kernel pages
            for (
              Size mappedBlockIndex = 0;
              mappedBlockIndex < blockIndex;
              mappedBlockIndex++
            ) {
              MemoryBlock freed
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      userBase + mappedBlockIndex * blockSize,
                      blockSize
                    }
                  );

              if (freed.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(freed);
              }
            }

            mapFailed = true;

            break;
          }

          MemoryBlock processBlock {
            userBase + blockIndex * blockSize,
            blockSize
          };
          MemoryBlock mappedBlock
            = _context->MemoryMapper->Map(
              *process->AddressSpace,
              processBlock,
              blockToMap,
              mapFlags
            );

          if (mappedBlock.SizeInBytes == 0) {
            _context->MemoryAllocator->Free(blockToMap);

            for (
              Size mappedBlockIndex = 0;
              mappedBlockIndex < blockIndex;
              mappedBlockIndex++
            ) {
              MemoryBlock freed
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      userBase + mappedBlockIndex * blockSize,
                      blockSize
                    }
                  );

              if (freed.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(freed);
              }
            }

            mapFailed = true;

            break;
          }
        }

        if (!mapFailed) {
          // record a single mapping covering the full allocation so
          // Memory_Free can find and release all pages in one operation
          process->AddressSpaceMap.Insert(
            MemoryMapping {
              process->AddressSpace,
              MemoryBlock { userBase, blockCount * blockSize },
              MemoryBlock {},
              mapFlags
            }
          );

          process->HeapBlockCount += blockCount;
          process->TotalBlockCount += blockCount;
        }

        if (mapFailed) {
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        // copy the IPCMessage struct into user space, with the Payload
        // pointer adjusted to point to the payload region that follows it
        auto* userMessage = reinterpret_cast<IPCMessage*>(userBase);

        userMessage->SendingProcessID = message->SendingProcessID;
        userMessage->PayloadSizeInBytes = message->PayloadSizeInBytes;
        userMessage->Payload = reinterpret_cast<void*>(
          userBase + sizeof(IPCMessage)
        );
        userMessage->Type = message->Type;
        userMessage->SharedDescriptor = message->SharedDescriptor;

        // copy the payload data into the user-space region after the
        // IPCMessage struct
        if (message->Payload && message->PayloadSizeInBytes > 0) {
          Byte::Copy(
            userMessage->Payload,
            message->Payload,
            message->PayloadSizeInBytes
          );
        }

        // free the kernel-side copies
        if (message->Payload) _context->HeapAllocator->Free(message->Payload);

        _context->IPCMessagePool->Free(message);

        context.EAX = userBase;

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: TryReceive (non-blocking)
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_TryReceive: {
        IPCPortResourceID handle
          = static_cast<IPCPortResourceID>(context.EBX);

        context.EAX = 0; // null on failure

        if (!process) break;

        Result<IPCPortResource*> getResourceResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResourceResult.Success) break;

        // verify receiver has Receive right
        if (!Enum::HasAnyFlag(getResourceResult.Data->Rights, IPCPortRights::Receive)) {
          break;
        }

        Result<IPCPort*> portResult
          = _context->IPCPortRepository->FindByID(getResourceResult.Data->ObjectID);

        if (!portResult.Success) break;

        // try to receive without blocking, returns immediately if empty
        Result<IPCMessage*> tryResult
          = _context->IPCPortService->TryReceive(portResult.Data);

        if (!tryResult.Success || !tryResult.Data) break;

        IPCMessage* message = tryResult.Data;

        // allocate user-space memory for the IPCMessage struct + payload
        Size totalSize = sizeof(IPCMessage) + message->PayloadSizeInBytes;
        Size blockSize = _context->MemoryAllocator->GetBlockSize();

        Result<MemoryBlock> userFindResult
          = process->AddressSpaceMap.FindAvailableBlock(
            totalSize,
            blockSize
          );

        if (!userFindResult.Success) {
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        UIntPtr userBase = userFindResult.Data.Base;
        Size blockCount = AlignUp<Size>(totalSize, blockSize) / blockSize;
        bool mapFailed = false;
        MemoryMappingFlags mapFlags {
          MemoryMappingPermissions::Read |
          MemoryMappingPermissions::Write |
          MemoryMappingPermissions::User,
          MemoryMappingCache::Default,
          MemoryMappingOptions::None
        };

        for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
          MemoryBlock blockToMap
            = _context->MemoryAllocator->Allocate(
                blockSize, MemoryBlockTag::IPCReceiveBuffer
              );

          if (blockToMap.Base == 0) {
            for (
              Size mappedBlockIndex = 0;
              mappedBlockIndex < blockIndex;
              mappedBlockIndex++
            ) {
              MemoryBlock freed
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      userBase + mappedBlockIndex * blockSize,
                      blockSize
                    }
                  );

              if (freed.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(freed);
              }
            }

            mapFailed = true;

            break;
          }

          MemoryBlock processBlock {
            userBase + blockIndex * blockSize,
            blockSize
          };
          MemoryBlock mappedBlock
            = _context->MemoryMapper->Map(
              *process->AddressSpace,
              processBlock,
              blockToMap,
              mapFlags
            );

          if (mappedBlock.SizeInBytes == 0) {
            _context->MemoryAllocator->Free(blockToMap);

            for (
              Size mappedBlockIndex = 0;
              mappedBlockIndex < blockIndex;
              mappedBlockIndex++
            ) {
              MemoryBlock freed
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      userBase + mappedBlockIndex * blockSize,
                      blockSize
                    }
                  );

              if (freed.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(freed);
              }
            }

            mapFailed = true;

            break;
          }
        }

        if (!mapFailed) {
          process->AddressSpaceMap.Insert(
            MemoryMapping {
              process->AddressSpace,
              MemoryBlock { userBase, blockCount * blockSize },
              MemoryBlock {},
              mapFlags
            }
          );

          process->HeapBlockCount += blockCount;
          process->TotalBlockCount += blockCount;
        }

        if (mapFailed) {
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        auto* userMessage = reinterpret_cast<IPCMessage*>(userBase);

        userMessage->SendingProcessID = message->SendingProcessID;
        userMessage->PayloadSizeInBytes = message->PayloadSizeInBytes;
        userMessage->Payload = reinterpret_cast<void*>(
          userBase + sizeof(IPCMessage)
        );
        userMessage->Type = message->Type;
        userMessage->SharedDescriptor = message->SharedDescriptor;

        if (message->Payload && message->PayloadSizeInBytes > 0) {
          Byte::Copy(
            userMessage->Payload,
            message->Payload,
            message->PayloadSizeInBytes
          );
        }

        if (message->Payload) _context->HeapAllocator->Free(message->Payload);

        _context->IPCMessagePool->Free(message);

        context.EAX = userBase;

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: ReceiveInto (zero-allocation receive into caller buffer)
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_ReceiveInto: {
        IPCPortResourceID handle = static_cast<IPCPortResourceID>(context.EBX);
        auto* userBuffer = reinterpret_cast<void*>(context.ECX);
        Size bufferSize = static_cast<Size>(context.EDX);

        context.EAX = 0;

        if (!process) break;
        if (!userBuffer || bufferSize < sizeof(IPCMessage)) break;

        // validate user buffer is within the process's mapped memory
        if (!_isValidUserPointer(
          process,
          reinterpret_cast<UIntPtr>(userBuffer),
          bufferSize
        )) break;

        Result<IPCPortResource*> getResourceResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResourceResult.Success) break;

        if (!Enum::HasAnyFlag(getResourceResult.Data->Rights, IPCPortRights::Receive)) {
          break;
        }

        Result<IPCPort*> portResult
          = _context->IPCPortRepository->FindByID(getResourceResult.Data->ObjectID);

        if (!portResult.Success) break;

        IPCMessage* message = _context->IPCPortService->Receive(portResult.Data);

        if (!message) break;

        Size totalSize = sizeof(IPCMessage) + message->PayloadSizeInBytes;

        if (totalSize > bufferSize) {
          // message too large for buffer, free kernel copies, return error
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        // copy directly into user buffer, no page allocation needed
        auto* userMessage = static_cast<IPCMessage*>(userBuffer);

        userMessage->SendingProcessID = message->SendingProcessID;
        userMessage->PayloadSizeInBytes = message->PayloadSizeInBytes;
        userMessage->Payload = reinterpret_cast<void*>(
          reinterpret_cast<UInt32>(userBuffer) + sizeof(IPCMessage)
        );
        userMessage->Type = message->Type;
        userMessage->SharedDescriptor = message->SharedDescriptor;

        if (message->Payload && message->PayloadSizeInBytes > 0) {
          Byte::Copy(
            userMessage->Payload,
            message->Payload,
            message->PayloadSizeInBytes
          );
        }

        if (message->Payload) _context->HeapAllocator->Free(message->Payload);

        _context->IPCMessagePool->Free(message);

        context.EAX = reinterpret_cast<UInt32>(userBuffer);

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: ReceiveAny (multiplexed wait on multiple ports)
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_ReceiveAny: {
        auto* userHandles
          = reinterpret_cast<IPCPortResourceID*>(context.EBX);
        Size handleCount = static_cast<Size>(context.ECX);

        context.EAX = 0;
        context.EDI = 0;

        if (!process) break;
        if (!userHandles || handleCount == 0 || handleCount > 16) break;

        // resolve handles to ports
        constexpr Size MaxPorts = 16;
        IPCPort* resolvedPorts[MaxPorts] = {};
        bool valid = true;

        for (Size i = 0; i < handleCount; ++i) {
          Result<IPCPortResource*> res
            = _context->KernelResourceManager->GetTyped<IPCPortResource>(
                userHandles[i],
                KernelResourceType::IPCPort
              );

          if (!res.Success) { valid = false; break; }

          if (!Enum::HasAnyFlag(res.Data->Rights, IPCPortRights::Receive)) {
            valid = false;

            break;
          }

          Result<IPCPort*> portRes
            = _context->IPCPortRepository->FindByID(res.Data->ObjectID);

          if (!portRes.Success) { valid = false; break; }

          resolvedPorts[i] = portRes.Data;
        }

        if (!valid) break;

        Size portIndex = 0;
        IPCMessage* message = _context->IPCPortService->ReceiveAny(
          resolvedPorts, handleCount, &portIndex
        );

        if (!message) break;

        // map the message to user space (same as IPC_Receive)
        Size totalSize = sizeof(IPCMessage) + message->PayloadSizeInBytes;
        Size blockSize = _context->MemoryAllocator->GetBlockSize();

        Result<MemoryBlock> userFindResult
          = process->AddressSpaceMap.FindAvailableBlock(
            totalSize,
            blockSize
          );

        if (!userFindResult.Success) {
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        UIntPtr userBase = userFindResult.Data.Base;
        Size blockCount = AlignUp<Size>(totalSize, blockSize) / blockSize;
        bool mapFailed = false;
        MemoryMappingFlags mapFlags {
          MemoryMappingPermissions::Read |
          MemoryMappingPermissions::Write |
          MemoryMappingPermissions::User,
          MemoryMappingCache::Default,
          MemoryMappingOptions::None
        };

        for (Size blockIndex = 0; blockIndex < blockCount; blockIndex++) {
          MemoryBlock blockToMap
            = _context->MemoryAllocator->Allocate(
                blockSize, MemoryBlockTag::IPCReceiveBuffer
              );

          if (blockToMap.Base == 0) {
            for (
              Size mappedBlockIndex = 0;
              mappedBlockIndex < blockIndex;
              mappedBlockIndex++
            ) {
              MemoryBlock freed
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      userBase + mappedBlockIndex * blockSize,
                      blockSize
                    }
                  );

              if (freed.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(freed);
              }
            }

            mapFailed = true;

            break;
          }

          MemoryBlock processBlock {
            userBase + blockIndex * blockSize,
            blockSize
          };
          MemoryBlock mappedBlock
            = _context->MemoryMapper->Map(
              *process->AddressSpace,
              processBlock,
              blockToMap,
              mapFlags
            );

          if (mappedBlock.SizeInBytes == 0) {
            _context->MemoryAllocator->Free(blockToMap);

            for (
              Size mappedBlockIndex = 0;
              mappedBlockIndex < blockIndex;
              mappedBlockIndex++
            ) {
              MemoryBlock freed
                = _context->MemoryMapper->Unmap(
                    *process->AddressSpace,
                    MemoryBlock {
                      userBase + mappedBlockIndex * blockSize,
                      blockSize
                    }
                  );

              if (freed.SizeInBytes > 0) {
                _context->MemoryAllocator->Free(freed);
              }
            }

            mapFailed = true;

            break;
          }
        }

        if (!mapFailed) {
          process->AddressSpaceMap.Insert(
            MemoryMapping {
              process->AddressSpace,
              MemoryBlock { userBase, blockCount * blockSize },
              MemoryBlock {},
              mapFlags
            }
          );

          process->HeapBlockCount += blockCount;
          process->TotalBlockCount += blockCount;
        }

        if (mapFailed) {
          if (message->Payload) _context->HeapAllocator->Free(message->Payload);

          _context->IPCMessagePool->Free(message);

          break;
        }

        auto* userMessage = reinterpret_cast<IPCMessage*>(userBase);

        userMessage->SendingProcessID = message->SendingProcessID;
        userMessage->PayloadSizeInBytes = message->PayloadSizeInBytes;
        userMessage->Payload = reinterpret_cast<void*>(
          userBase + sizeof(IPCMessage)
        );
        userMessage->Type = message->Type;
        userMessage->SharedDescriptor = message->SharedDescriptor;

        if (message->Payload && message->PayloadSizeInBytes > 0) {
          Byte::Copy(
            userMessage->Payload,
            message->Payload,
            message->PayloadSizeInBytes
          );
        }

        if (message->Payload) _context->HeapAllocator->Free(message->Payload);

        _context->IPCMessagePool->Free(message);

        context.EAX = userBase;
        context.EDI = static_cast<UInt32>(portIndex);

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: Transfer (UNTESTED)
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_Transfer: {
        IPCPortResourceID handle = static_cast<IPCPortResourceID>(context.EBX);
        ProcessID newOwnerPID = static_cast<ProcessID>(context.ECX);

        context.EAX = 0; // failure

        if (newOwnerPID == 0) break;

        Result<IPCPortResource*> getResult
          = _context->KernelResourceManager->GetTyped<IPCPortResource>(
              handle,
              KernelResourceType::IPCPort
            );

        if (!getResult.Success) break;

        IPCPortResource* resource = getResult.Data;

        // check if current process can manage this resource
        Result<IPCPort*> portResult
          = _context->IPCPortRepository->FindByID(resource->ObjectID);

        if (!portResult.Success) break;

        IPCPort* port = portResult.Data;
        bool canManage =
          (port->OwnerProcess && port->OwnerProcess->ID == currentPID)
          || Enum::HasAnyFlag(resource->Rights, IPCPortRights::Manage);

        if (!canManage) break;

        Process* newOwner = _context->ProcessesManager->GetByID(newOwnerPID);

        if (!newOwner) break;

        // transfer port ownership if the resource has Manage right
        if (Enum::HasAnyFlag(resource->Rights, IPCPortRights::Manage)) {
          port->OwnerProcess = newOwner;
        }

        resource->OwnerPID = newOwnerPID;

        context.EAX = 1; // success

        break;
      }

      // -----------------------------------------------------------------------
      // Thread: Yield
      // -----------------------------------------------------------------------
      case KernelOperation::Thread_Yield: {
        return static_cast<IA32InterruptContext*>(
          _context->ThreadManager->Yield(&context)
        );
      }

      // -----------------------------------------------------------------------
      // Thread: Sleep
      // -----------------------------------------------------------------------
      case KernelOperation::Thread_Sleep: {
        UInt32 ticks = context.EBX;

        if (ticks > 0) {
          Thread* current = _context->ThreadManager->GetCurrent();

          if (current) _context->ThreadManager->Sleep(current, ticks);
        }

        break;
      }

      case KernelOperation::Thread_Create: {
        UIntPtr entryPoint = context.EBX;
        UInt32 argument = context.ECX;

        if (entryPoint == 0 || !process) break;

        Stack* stack = _context->Stacks->Create(
          *process->AddressSpace,
          THREAD_STACK_SIZE,
          MemoryMappingPermissions::User
        );

        if (!stack || stack->Base == 0) break;

        Size blockSize = _context->MemoryAllocator->GetBlockSize();
        Size stackBlockCount = stack->SizeInBytes / blockSize;

        for (Size i = 0; i < stackBlockCount; i++) {
          process->AddressSpaceMap.Insert(
            MemoryMapping {
              process->AddressSpace,
              MemoryBlock {
                stack->Base + i * blockSize,
                blockSize
              },
              MemoryBlock {},
              MemoryMappingFlags {
                MemoryMappingPermissions::Read |
                MemoryMappingPermissions::Write |
                MemoryMappingPermissions::User,
                MemoryMappingCache::Default,
                MemoryMappingOptions::None
              },
              MemoryRegionType::Stack
            }
          );
        }

        process->TotalBlockCount += stackBlockCount;

        stack->Push<UInt32>(argument);
        stack->Push<UInt32>(0);

        Thread* newThread = _context->ProcessesManager->CreateUserThread(
          process,
          process->Name,
          entryPoint,
          stack->Top,
          ThreadPriority::UserNormal
        );

        if (!newThread) break;

        newThread->UserStack = stack;

        if (!_context->ThreadManager->Start(newThread)) break;

        context.EAX = newThread->ID;

        break;
      }

      case KernelOperation::Thread_FutexWait: {
        UIntPtr address = context.EBX;
        UInt32 expected = context.ECX;

        if (address == 0) break;

        Thread* current = _context->ThreadManager->GetCurrent();

        if (!current) break;

        bool queued = _context->FutexManager->Wait(address, expected, current);

        if (queued) {
          context.EAX = 0;

          // indefinite sleep, immediately yields via Yield;
          // Tick sees State != Running and doesn't re-enqueue;
          // FutexWake calls Resume to wake the thread
          _context->ThreadManager->Sleep(current, 0);

          break;
        }

        context.EAX = 1;

        break;
      }

      case KernelOperation::Thread_FutexWake: {
        UIntPtr address = context.EBX;
        UInt32 maxCount = context.ECX;

        if (address == 0 || maxCount == 0) break;

        context.EAX = static_cast<UInt32>(
          _context->FutexManager->Wake(address, static_cast<Size>(maxCount))
        );

        break;
      }

      case KernelOperation::Thread_SetPriority: {
        UInt32 pri = context.EBX;

        if (pri > Enum::ToBase(ThreadPriority::Maximum)) {
          context.EAX = 1;
          break;
        }

        auto priority = static_cast<ThreadPriority>(pri);
        Thread* current = _context->ThreadManager->GetCurrent();
        current->BasePriority = priority;
        context.EAX = 0;

        break;
      }

      // -----------------------------------------------------------------------
      // Thread: Exit
      // -----------------------------------------------------------------------
      case KernelOperation::Thread_Exit: {
        Int32 exitCode = static_cast<Int32>(context.EBX);

        // if this is the last thread, terminate the process instead
        if (process && process->ThreadCount <= 1) {
          _context->ProcessesManager->Terminate(process, exitCode);

          return static_cast<IA32InterruptContext*>(
            _context->ThreadManager->Terminate(&context, exitCode)
          );
        }

        return static_cast<IA32InterruptContext*>(
          _context->ThreadManager->Terminate(&context, exitCode)
        );
      }

      // -----------------------------------------------------------------------
      // PortIO: In8
      // -----------------------------------------------------------------------
      case KernelOperation::PortIO_In8: {
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::PortIO
          )
        ) break;

        UInt16 port = static_cast<UInt16>(context.EBX);

        context.EAX = static_cast<UInt32>(_context->CPU->In8(port));

        break;
      }

      // -----------------------------------------------------------------------
      // PortIO: In16
      // -----------------------------------------------------------------------
      case KernelOperation::PortIO_In16: {
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::PortIO
          )
        ) break;

        UInt16 port = static_cast<UInt16>(context.EBX);

        context.EAX = static_cast<UInt32>(_context->CPU->In16(port));

        break;
      }

      // -----------------------------------------------------------------------
      // PortIO: In32
      // -----------------------------------------------------------------------
      case KernelOperation::PortIO_In32: {
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::PortIO
          )
        ) break;

        UInt16 port = static_cast<UInt16>(context.EBX);

        context.EAX = _context->CPU->In32(port);

        break;
      }

      // -----------------------------------------------------------------------
      // PortIO: Out8
      // -----------------------------------------------------------------------
      case KernelOperation::PortIO_Out8: {
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::PortIO
          )
        ) break;

        UInt16 port = static_cast<UInt16>(context.EBX);
        UInt8 value = static_cast<UInt8>(context.ECX);

        _context->CPU->Out8(port, value);

        break;
      }

      // -----------------------------------------------------------------------
      // PortIO: Out16
      // -----------------------------------------------------------------------
      case KernelOperation::PortIO_Out16: {
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::PortIO
          )
        ) break;

        UInt16 port = static_cast<UInt16>(context.EBX);
        UInt16 value = static_cast<UInt16>(context.ECX);

        _context->CPU->Out16(port, value);

        break;
      }

      // -----------------------------------------------------------------------
      // PortIO: Out32
      // -----------------------------------------------------------------------
      case KernelOperation::PortIO_Out32: {
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::PortIO
          )
        ) break;

        UInt16 port = static_cast<UInt16>(context.EBX);
        UInt32 value = context.ECX;

        _context->CPU->Out32(port, value);

        break;
      }

      // -----------------------------------------------------------------------
      // Interrupt: Claim
      // -----------------------------------------------------------------------
      case KernelOperation::Interrupt_Claim: {
        UInt8 irq = static_cast<UInt8>(context.EBX);

        context.EAX = static_cast<UInt32>(-1); // failure

        if (!process) break;

        // check that the process has the Interrupts permission
        if (
          !_context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::Interrupts
          )
        ) break;

        // validate IRQ number
        if (irq >= IRQ_COUNT) break;

        // check that the IRQ is not already claimed
        Result<IA32InterruptResource*> existingResult
          = _context->KernelResourceManager->FindByObjectID<IA32InterruptResource>(
              KernelResourceType::Interrupt,
              irq
            );

        if (existingResult.Success) break;

        // create the interrupt resource
        Result<IA32InterruptResource*> createResult
          = _context->KernelResourceManager->Create<IA32InterruptResource>(
              KernelResourceType::Interrupt,
              irq,
              static_cast<UInt32>(0),
              currentPID
            );

        if (!createResult.Success) break;

        // register the shared IRQ handler and unmask the IRQ
        _context->InterruptManager->SetHandler(
          IRQ_BASE_VECTOR + irq,
          IA32UserInterruptHandler::Handle
        );
        _context->InterruptController->Unmask(irq);

        KLOG_TRACE(
          "PID %u claimed IRQ %u (resource ID %u)",
          currentPID,
          static_cast<UInt32>(irq),
          createResult.Data->ID
        );

        context.EAX = createResult.Data->ID;

        break;
      }

      // -----------------------------------------------------------------------
      // Interrupt: Wait
      // -----------------------------------------------------------------------
      case KernelOperation::Interrupt_Wait: {
        ResourceID handle = static_cast<ResourceID>(context.EBX);

        if (
          process &&
          _context->ProcessesManager->HasPermissions(
            currentPID,
            ProcessPermissions::Interrupts
          )
        ) {
          Result<IA32InterruptResource*> getResult
            = _context->KernelResourceManager->GetTyped<IA32InterruptResource>(
                handle,
                KernelResourceType::Interrupt
              );

          if (getResult.Success) {
            IA32InterruptResource* resource = getResult.Data;

            // verify that this process owns the resource
            if (resource->OwnerPID == currentPID) {
              // if an interrupt arrived while we weren't waiting, consume it
              if (resource->PendingCount > 0) {
                resource->PendingCount--;

                break;
              } else {
                // block until the next interrupt fires
                resource->WaitingThread = _context->ThreadManager->GetCurrent();

                _context->ThreadManager->Sleep(
                  _context->ThreadManager->GetCurrent(),
                  0
                );
              }
            }
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Interrupt: Release
      // -----------------------------------------------------------------------
      case KernelOperation::Interrupt_Release: {
        ResourceID handle = static_cast<ResourceID>(context.EBX);

        if (process) {
          Result<IA32InterruptResource*> getResult
            = _context->KernelResourceManager->GetTyped<IA32InterruptResource>(
                handle,
                KernelResourceType::Interrupt
              );

          if (getResult.Success) {
            IA32InterruptResource* resource = getResult.Data;

            // verify that this process owns the resource
            if (resource->OwnerPID == currentPID) {
              UInt8 irq = resource->ObjectID;

              // mask the IRQ and remove the handler
              _context->InterruptController->Mask(irq);
              _context->InterruptManager->SetHandler(
                IRQ_BASE_VECTOR + irq,
                nullptr
              );

              // remove and delete the resource
              _context->KernelResourceManager->Remove(handle);

              KLOG_TRACE(
                "PID %u released IRQ %u (resource ID %u)",
                currentPID,
                static_cast<UInt32>(irq),
                handle
              );

              delete resource;
            }
          }
        }

        break;
      }

      // -----------------------------------------------------------------------
      // IPC: WaitForPort
      // -----------------------------------------------------------------------
      case KernelOperation::IPC_WaitForPort: {
        IPCPortID portID = static_cast<IPCPortID>(context.EBX);

        context.EAX = 0;

        if (
          process &&
          _context->IPCPortRepository->WaitForPort(portID)
        ) {
          context.EAX = 1;
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Scheduler: GetStats
      // -----------------------------------------------------------------------
      case KernelOperation::Scheduler_GetStats: {
        SchedulerStats* stats = reinterpret_cast<SchedulerStats*>(context.EBX);

        context.EAX = 0;

        if (!stats) break;

        if (
          _isValidUserPointer(
            process,
            context.EBX,
            sizeof(SchedulerStats)
          )
        ) {
          _context->Scheduler->GetStats(stats);

          context.EAX = 1;
        }

        break;
      }

      // -----------------------------------------------------------------------
      // Unknown
      // -----------------------------------------------------------------------
      default: {
        _context->Log->Warning(
          "Unknown system call operation %u",
          static_cast<UInt32>(operation),
          context.EBX
        );

        break;
      }
    }

    return &context;
  }

  bool IA32SystemCallHandler::_isValidUserPointer(
    Process* process,
    UIntPtr address,
    Size sizeInBytes
  ) {
    if (
      !process ||
      address == 0 ||
      sizeInBytes == 0 ||
      address + sizeInBytes < address
    ) {
      return false;
    } else {
      // verify the range falls within a mapped region owned by the process;
      // this is the authoritative check, thread stacks live above the
      // kernel higher-half base (0xE2...) but are still valid user memory
      Result<MemoryMapping> result
        = process->AddressSpaceMap.FindContainingBlock(address);

      if (result.Success) {
        // ensure the entire range fits within the mapping
        UIntPtr mappingEnd
          = result.Data.ProcessBlock.Base
          + result.Data.ProcessBlock.SizeInBytes;

        return (address + sizeInBytes) <= mappingEnd;
      } else {
        return false;
      }
    }
  }

  bool IA32SystemCallHandler::_isValidUserString(
    Process* process,
    UIntPtr address
  ) {
    // validate that at least the first byte is in a valid user mapping;
    // full string length validation would require scanning for the null
    // terminator, which is expensive, this catches the common case of
    // wild pointers and kernel-space addresses
    return _isValidUserPointer(process, address, 1);
  }
}
