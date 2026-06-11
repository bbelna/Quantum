/**
 * @file Kernel/Arch/IA32/Memory/IA32MemoryMapper.cpp
 * @brief Implements @ref @QKrnlIA32::Memory::IA32MemoryMapper.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/IA32LinkerSymbols.hpp>
#include <Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp>
#include <KernelLog.hpp>
#include <Memory/MemoryMappingFlags.hpp>

#include "IA32MemoryMapper.hpp"
#include "ToIA32PageTableFlags.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  void IA32MemoryMapper::Initialize(
    IA32CPUDriver* cpu,
    IMemoryAllocator* memoryAllocator,
    IA32PageDirectoryManager* pageDirectoryManager
  ) {
    _cpu = cpu;
    _memoryAllocator = memoryAllocator;
    _pageDirectoryManager = pageDirectoryManager;

    UInt32 kernelSizeInBytes
      = KERNEL_PHYSICAL_END
      - KERNEL_HIGHER_HALF_PHYSICAL_BASE;

    KLOG_TRACE(
      "Kernel spans %u pages",
      kernelSizeInBytes / PAGE_SIZE
    );

    // map the kernel higher half
    for (
      UInt32 offset = 0;
      offset < kernelSizeInBytes;
      offset += PAGE_SIZE
    ) {
      UIntPtr higherHalfPhysicalBase
        = KERNEL_HIGHER_HALF_PHYSICAL_BASE
        + offset;
      UIntPtr higherHalfVirtualBase
        = KERNEL_HIGHER_HALF_VIRTUAL_BASE
        + offset;

      Map(
        *pageDirectoryManager->GetKernelPageDirectory(),
        MemoryBlock {
          higherHalfVirtualBase,
          PAGE_SIZE
        },
        MemoryBlock {
          higherHalfPhysicalBase,
          PAGE_SIZE
        },
        MemoryMappingFlags {
          MemoryMappingPermissions::Write,
          MemoryMappingCache::Default,
          MemoryMappingOptions::Global
        }
      );
    }
  }

  MemoryBlock IA32MemoryMapper::Map(
    IAddressSpace& addressSpace,
    MemoryBlock blockToMap,
    const MemoryMappingFlags& flags
  ) {
    if (blockToMap.SizeInBytes != PAGE_SIZE) {
      MemoryBlock block = _memoryAllocator->Allocate();

      return block.SizeInBytes < blockToMap.SizeInBytes
        ? block
        : Map(
            addressSpace,
            blockToMap,
            block,
            flags
          );
    } else {
      return MemoryBlock {};
    }
  }

 MemoryBlock IA32MemoryMapper::Map(
    IAddressSpace& addressSpace,
    MemoryBlock virtualBlock,
    MemoryBlock kernelBlock,
    const MemoryMappingFlags& flags
  ) {
    auto& pageDirectory = static_cast<IA32PageDirectory&>(addressSpace);

    if (
      !_pageDirectoryManager ||
      virtualBlock.SizeInBytes == 0 ||
      kernelBlock.SizeInBytes == 0 ||
      virtualBlock.SizeInBytes != kernelBlock.SizeInBytes ||
      virtualBlock.SizeInBytes % PAGE_SIZE != 0
    ) return MemoryBlock { 0, 0 };

    IA32PageTableFlags pageFlags = ToIA32PageTableFlags(flags.Permissions);

    pageFlags |= IA32PageTableFlags::Present;

    // set page flags based on mapping options

    if (
      Enum::HasFlag(
        flags.Permissions,
        MemoryMappingPermissions::User
      )
    ) {
      pageFlags |= IA32PageTableFlags::User;
    }

    if (
      Enum::HasFlag(
        flags.Permissions,
        MemoryMappingPermissions::Write
      )
    ) {
      pageFlags |= IA32PageTableFlags::Write;
    }

    if (
      Enum::HasFlag(
        flags.Options,
        MemoryMappingOptions::Global
      )
    ) {
      pageFlags |= IA32PageTableFlags::Global;
    }

    // apply cache attribute flags
    switch (flags.Cache) {
      case MemoryMappingCache::WriteThrough: {
        pageFlags |= IA32PageTableFlags::WriteThrough;

        break;
      }

      case MemoryMappingCache::Uncached: {
        pageFlags
          |= IA32PageTableFlags::CacheDisable
           | IA32PageTableFlags::WriteThrough;

        break;
      }

      case MemoryMappingCache::WriteCombining: {
        if (_cpu->IsPATSupported()) {
          pageFlags |= IA32PageTableFlags::PAT;
        } else {
          pageFlags |= IA32PageTableFlags::CacheDisable;
        }

        break;
      }

      case MemoryMappingCache::Default:
      case MemoryMappingCache::WriteBack:
      default: {
        break;
      }
    }

    UInt32 pageCount = static_cast<UInt32>(
      virtualBlock.SizeInBytes / PAGE_SIZE
    );

    for (
      UInt32 pageIndex = 0;
      pageIndex < pageCount;
      ++pageIndex
    ) {
      UIntPtr address = kernelBlock.Base + pageIndex * PAGE_SIZE;
      UIntPtr processAddress = virtualBlock.Base + pageIndex * PAGE_SIZE;
      UInt32 pageDirectoryIndex = IA32PageDirectory::Index(processAddress);
      UInt32 pageTableIndex = IA32PageTable::Index(processAddress);
      IA32PageTable* pageTable = _pageDirectoryManager->EnsureTable(
        &pageDirectory,
        pageDirectoryIndex
      );

      if (pageTable->Entries[pageTableIndex].IsPresent()) {
        // allow overwriting identity-mapped entries (frame base == process
        // address); these are default entries from the deep-copied identity
        // map and are expected to be replaced by real user-space mappings
        UIntPtr identityFrame = processAddress & ~static_cast<UIntPtr>(0xFFF);

        if (pageTable->Entries[pageTableIndex].GetFrameBase() != identityFrame) {
          KLOG_ERROR(
            "Map: process %p already mapped (PTE %p), refusing to overwrite",
            processAddress,
            pageTable->Entries[pageTableIndex].Value
          );

          return MemoryBlock { 0, 0 };
        }
      }

      pageTable->Entries[pageTableIndex].SetFrameBase(address);
      pageTable->Entries[pageTableIndex].EnableFlags(pageFlags);

      if (
        Enum::HasFlag(
          flags.Permissions,
          MemoryMappingPermissions::User
        )
      ) {
        pageDirectory.Entries[pageDirectoryIndex].SetUser(true);
      }

      _cpu->InvalidatePage(processAddress);
    }

    KLOG_DEBUG(
      "Mapped process %p-%p to physical %p-%p with flags %u",
      virtualBlock.Base,
      virtualBlock.Base + virtualBlock.SizeInBytes - 1,
      kernelBlock.Base,
      kernelBlock.Base + kernelBlock.SizeInBytes - 1,
      static_cast<UInt32>(pageFlags)
    );

    return kernelBlock;
  }

  MemoryBlock IA32MemoryMapper::Unmap(
    IAddressSpace& addressSpace,
    MemoryBlock block
  ) {
    auto& pd = static_cast<IA32PageDirectory&>(addressSpace);

    if (
      !_pageDirectoryManager ||
      block.SizeInBytes == 0 ||
      block.SizeInBytes % PAGE_SIZE != 0
    ) {
      return MemoryBlock { 0, 0 };
    }

    MemoryBlock lastUnmapped { 0, 0 };
    UInt32 pageCount = static_cast<UInt32>(block.SizeInBytes / PAGE_SIZE);

    for (UInt32 i = 0; i < pageCount; ++i) {
      UIntPtr processAddress = block.Base + i * PAGE_SIZE;
      UInt32 pageDirectoryIndex = IA32PageDirectory::Index(processAddress);
      UInt32 pageTableIndex = IA32PageTable::Index(processAddress);
      IA32PageDirectoryEntry& pageDirectoryEntry
        = pd.Entries[pageDirectoryIndex];

      if (!pageDirectoryEntry.IsPresent()) continue;

      IA32PageTable* pageTable
        = reinterpret_cast<IA32PageTable*>(pageDirectoryEntry.GetTableBase());
      IA32PageTableEntry& pageTableEntry = pageTable->Entries[pageTableIndex];

      if (!pageTableEntry.IsPresent()) continue;

      UIntPtr physicalAddress = pageTableEntry.GetFrameBase();

      pageTableEntry.Clear();

      _cpu->InvalidatePage(processAddress);

      KLOG_DEBUG(
        "Unmapped process %p previously mapped to physical %p",
        processAddress,
        physicalAddress
      );

      lastUnmapped.Base = physicalAddress;
      lastUnmapped.SizeInBytes = PAGE_SIZE;
    }

    return lastUnmapped;
  }

  MemoryBlock IA32MemoryMapper::Resolve(
    IAddressSpace& addressSpace,
    MemoryBlock block
  ) {
    IA32PageDirectory& pageDirectory = static_cast<IA32PageDirectory&>(
      addressSpace
    );
    MemoryBlock foundBlock {};

    if (
      block.SizeInBytes >= 0 &&
      block.SizeInBytes % PAGE_SIZE >= 0
    ) {
      // resolve the first block's address
      UInt32 pageDirectoryIndex = IA32PageDirectory::Index(block.Base);
      UInt32 pageTableIndex = IA32PageTable::Index(block.Base);
      IA32PageDirectoryEntry& pageDirectoryEntry = pageDirectory.Entries[
        pageDirectoryIndex
      ];

      if (pageDirectoryEntry.IsPresent()) {
        IA32PageTable* pageTable = reinterpret_cast<IA32PageTable*>(
          pageDirectoryEntry.GetTableBase()
        );
        IA32PageTableEntry& pageTableEntry = pageTable->Entries[pageTableIndex];

        if (pageTableEntry.IsPresent()) {
          foundBlock = MemoryBlock {
            pageTableEntry.GetFrameBase(),
            PAGE_SIZE
          };
        }
      }
    }

    return foundBlock;
  }
}
