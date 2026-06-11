/**
 * @file Kernel/Arch/IA32/Memory/IA32PageDirectoryManager.cpp
 * @brief Implements @ref @QKrnlIA32::Memory::PageDirectoryManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp>
#include <Arch/IA32/IA32Constants.hpp>
#include <Arch/IA32/IA32LinkerSymbols.hpp>
#include <KernelLog.hpp>

#include "IA32PageDirectoryManager.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  void IA32PageDirectoryManager::Initialize(
    IA32CPUDriver* cpu,
    IA32MemoryAllocator* memoryAllocator
  ) {
    _cpu = cpu;
    _memoryAllocator = memoryAllocator;

    _initializeKernelPageDirectory();
  }

  IA32PageDirectory* IA32PageDirectoryManager::Allocate() {
    MemoryBlock pageDirectoryBlock = _memoryAllocator->Allocate(
      MemoryBlockTag::PageTable
    );

    if (pageDirectoryBlock.Base == 0) {
      KLOG_ERROR("Failed to allocate block for page directory");

      return nullptr;
    }

    IA32PageDirectory* pageDirectory = reinterpret_cast<IA32PageDirectory*>(
      pageDirectoryBlock.Base
    );

    if (!_copyKernelPageDirectoriesTo(pageDirectory)) {
      _memoryAllocator->Free(pageDirectoryBlock);

      KLOG_ERROR("Failed to clone kernel page tables for new process");

      return nullptr;
    }

    KLOG_TRACE(
      "Allocated new page directory %p at %p",
      pageDirectory,
      pageDirectoryBlock.Base
    );

    return pageDirectory;
  }

  void IA32PageDirectoryManager::Free(IAddressSpace* addressSpace) {
    IA32PageDirectory* pageDirectory = static_cast<IA32PageDirectory*>(
      addressSpace
    );

    // do not free null or kernel page directory
    if (
      pageDirectory &&
      pageDirectory != _kernelPageDirectory
    ) {
      UInt32 kernelPageDirectoryStartIndex = IA32PageDirectory::Index(
        KERNEL_VIRTUAL_BASE
      );

      // free user-space page tables (below kernel virtual base)
      // we don't free kernel-space tables since those are shared
      for (
        UInt32 pageDirectoryEntryIndex = 0;
        pageDirectoryEntryIndex < kernelPageDirectoryStartIndex;
        ++pageDirectoryEntryIndex
      ) {
        IA32PageDirectoryEntry directoryEntry
          = pageDirectory->Entries[pageDirectoryEntryIndex];

        if (!directoryEntry.IsPresent()) {
          continue;
        }

        UIntPtr tableBase
          = _kernelPageDirectory
              ->Entries[pageDirectoryEntryIndex]
               .GetTableBase();
        bool isClonedTable = directoryEntry.GetTableBase() != tableBase;

        IA32PageTable* table = reinterpret_cast<IA32PageTable*>(
          directoryEntry.GetTableBase()
        );

        // free kernel frames that were mapped for this process
        // skip any frame that the kernel's own page table maps at the same
        // virtual slot, these are shared kernel mappings (identity map,
        // startup bundle, etc.) that are not process-owned; only free frames
        // that the process exclusively owns (different from the kernel's entry)
        IA32PageTable* kernelTable
          = _kernelPageDirectory
              ->Entries[pageDirectoryEntryIndex]
               .IsPresent()
          ? reinterpret_cast<IA32PageTable*>(
              _kernelPageDirectory
                ->Entries[pageDirectoryEntryIndex]
                 .GetTableBase()
            )
          : nullptr;

        for (
          UInt32 pageTableEntryIndex = 0;
          pageTableEntryIndex < PAGE_TABLE_ENTRY_COUNT;
          ++pageTableEntryIndex
        ) {
          IA32PageTableEntry tableEntry = table->Entries[pageTableEntryIndex];

          if (!tableEntry.IsPresent()) {
            continue;
          }

          // if the kernel's table has the same physical frame at this slot,
          // the page belongs to a shared kernel mapping, do not free it
          if (
            kernelTable &&
            kernelTable->Entries[pageTableEntryIndex].GetFrameBase()
              == tableEntry.GetFrameBase()
          ) continue;

          _memoryAllocator->Free(
            MemoryBlock {
              tableEntry.GetFrameBase(),
              PAGE_SIZE
            }
          );
        }

        // free the cloned page table structure itself
        if (isClonedTable) {
          _memoryAllocator->Free(
            MemoryBlock {
              directoryEntry.GetTableBase(),
              PAGE_SIZE
            }
          );
        }
      }

      // free the page directory itself
      _memoryAllocator->Free(
        MemoryBlock {
          reinterpret_cast<UIntPtr>(pageDirectory),
          PAGE_SIZE
        }
      );

      KLOG_TRACE(
        "Freed page directory at %p",
        pageDirectory
      );
    }
  }

  IA32PageTable* IA32PageDirectoryManager::EnsureTable(
    IA32PageDirectory* pageDirectory,
    UInt32 pageDirectoryIndex
  ) {
    IA32PageDirectoryEntry* pageDirectoryEntry
      = &pageDirectory->Entries[pageDirectoryIndex];

    if (pageDirectoryEntry->IsPresent()) {
      return reinterpret_cast<IA32PageTable*>(
        pageDirectoryEntry->GetTableBase()
      );
    }

    UIntPtr tableAddress;
    IA32PageTable* table = nullptr;
    MemoryBlock block = _memoryAllocator->Allocate(MemoryBlockTag::PageTable);

    tableAddress = block.Base;
    table = reinterpret_cast<IA32PageTable*>(tableAddress);

    for (
      UInt32 tableEntryIndex = 0;
      tableEntryIndex < PAGE_TABLE_ENTRY_COUNT;
      ++tableEntryIndex
    ) {
      table->Entries[tableEntryIndex].Clear();
    }

    pageDirectoryEntry->SetTableBase(tableAddress);
    pageDirectoryEntry->SetPresent(true);
    pageDirectoryEntry->SetWritable(true);

    KLOG_DEBUG(
      "Allocated page table %p for directory %p at index %u",
      tableAddress,
      pageDirectory,
      pageDirectoryIndex
    );

    return reinterpret_cast<IA32PageTable*>(tableAddress);
  }

  void IA32PageDirectoryManager::_initializeKernelPageDirectory() {
    static IA32PageDirectory kernelPageDirectory;

    _kernelPageDirectory = &kernelPageDirectory;

    UInt32 managedBytes = _memoryAllocator->GetManagedBytes();
    UInt32 managedBytesPerPageTable = PAGE_TABLE_ENTRY_COUNT * PAGE_SIZE;
    UInt32 pageTablesNeeded = static_cast<UInt32>(
      (
        managedBytes + (managedBytesPerPageTable - 1)
      ) / managedBytesPerPageTable
    );

    if (pageTablesNeeded > 1024) {
      pageTablesNeeded = 1024;
    }

    KLOG_DEBUG(
      "Initializing kernel page directory for %u MB using %u tables",
      managedBytes / (MB),
      pageTablesNeeded
    );

    for (
      UInt32 pageTableIndex = 0;
      pageTableIndex < pageTablesNeeded;
      ++pageTableIndex
    ) {
      IA32PageTable* table = EnsureTable(
        _kernelPageDirectory,
        pageTableIndex
      );
      UInt32 base = pageTableIndex * managedBytesPerPageTable;

      for (
        UInt32 pageTableEntry = 0;
        pageTableEntry < PAGE_TABLE_ENTRY_COUNT;
        ++pageTableEntry
      ) {
        table->Entries[pageTableEntry].SetFrameBase(
          base + pageTableEntry * PAGE_SIZE
        );
        table->Entries[pageTableEntry].SetPresent(true);
        table->Entries[pageTableEntry].SetWritable(true);
      }

      if (pageTableIndex == 0) {
        table->Entries[0].Clear(); // guard null page
      }

      _kernelPageDirectory->Entries[pageTableIndex].SetTableBase(
        _cpu->VirtualToPhysical(
          reinterpret_cast<UIntPtr>(table)
        )
      );
      _kernelPageDirectory->Entries[pageTableIndex].SetPresent(true);
      _kernelPageDirectory->Entries[pageTableIndex].SetWritable(true);

      KLOG_DEBUG(
        "Set page directory entry %u to table %p",
        pageTableIndex,
        _cpu->VirtualToPhysical(
          reinterpret_cast<UIntPtr>(table)
        )
      );
    }

    _initializeKernelHeapPageTables();

    // reserve kernel page tables so they cannot be reused by the allocator
    for (
      UInt32 pageDirectoryEntryIndex = 0;
      pageDirectoryEntryIndex < PAGE_DIRECTORY_ENTRY_COUNT;
      ++pageDirectoryEntryIndex
    ) {
      IA32PageDirectoryEntry entry
        = _kernelPageDirectory->Entries[pageDirectoryEntryIndex];

      if (!entry.IsPresent()) {
        continue;
      }

      UIntPtr tableAddress = entry.GetTableBase();

      _memoryAllocator->Reserve(
        MemoryBlock {
          AlignDown(
            tableAddress,
            PAGE_SIZE
          ),
          PAGE_SIZE
        }
      );
    }

    // install recursive mapping in the last PDE
    UIntPtr pageDirectoryAddress = _cpu->VirtualToPhysical(
      reinterpret_cast<UIntPtr>(_kernelPageDirectory)
    );

    _kernelPageDirectory->Entries[PAGE_TABLE_RECURSIVE_SLOT].SetTableBase(
      pageDirectoryAddress
    );
    _kernelPageDirectory->Entries[PAGE_TABLE_RECURSIVE_SLOT].SetPresent(true);
    _kernelPageDirectory->Entries[PAGE_TABLE_RECURSIVE_SLOT].SetWritable(true);

    KLOG_TRACE(
      "Kernel page directory initialized for ~%uMB using %u tables",
      managedBytes / (MB),
      pageTablesNeeded
    );
  }

  void IA32PageDirectoryManager::_initializeKernelHeapPageTables() {
    UInt32 startIndex = IA32PageDirectory::Index(HEAP_BASE);
    UInt32 endIndex = IA32PageDirectory::Index(
      HEAP_BASE + HEAP_SIZE_IN_BYTES - 1
    );

    for (
      UInt32 index = startIndex;
      index <= endIndex;
      ++index
    ) {
      EnsureTable(
        _kernelPageDirectory,
        index
      );
    }

    KLOG_TRACE(
      "Kernel heap page tables initialized (%u to %u)",
      startIndex,
      endIndex
    );
  }

  bool IA32PageDirectoryManager::_copyKernelPageDirectoriesTo(
    IA32PageDirectory* newPageDirectory
  ) {
    if (newPageDirectory) {
      UInt32 kernelPageDirectoryStartIndex = IA32PageDirectory::Index(
        KERNEL_VIRTUAL_BASE
      );

      // copy kernel-space page directory entries (higher half)
      // these entries point to shared kernel page tables, so all address spaces
      // see the same kernel mappings
      for (
        UInt32 kernelDirectoryIndex = kernelPageDirectoryStartIndex;
        kernelDirectoryIndex < PAGE_TABLE_RECURSIVE_SLOT;
        ++kernelDirectoryIndex
      ) {
        newPageDirectory->Entries[kernelDirectoryIndex]
          = _kernelPageDirectory->Entries[kernelDirectoryIndex];
      }

      // deep-copy identity-mapped low memory page tables so each process has
      // its own independent copy; sharing the kernel's page tables would let
      // one process' user-space mappings (e.g. heap at 0x1000) overwrite
      // another process' view of the same process address
      for (
        UInt32 pageDirectoryEntryIndex = 0;
        pageDirectoryEntryIndex < kernelPageDirectoryStartIndex;
        ++pageDirectoryEntryIndex
      ) {
        if (
          !_kernelPageDirectory
            ->Entries[pageDirectoryEntryIndex]
             .IsPresent()
        ) {
          newPageDirectory->Entries[pageDirectoryEntryIndex]
            = _kernelPageDirectory->Entries[pageDirectoryEntryIndex];

          continue;
        }

        // allocate a fresh page table and copy entries from the kernel's table
        MemoryBlock tableBlock = _memoryAllocator->Allocate(
          MemoryBlockTag::PageTable
        );

        if (tableBlock.Base == 0) {
          KLOG_ERROR(
            "Failed to allocate page table clone for PDE %u",
            pageDirectoryEntryIndex
          );

          // roll back: free any cloned tables we already allocated
          for (
            UInt32 rollbackIndex = 0;
            rollbackIndex < pageDirectoryEntryIndex;
            ++rollbackIndex
          ) {
            if (
              _kernelPageDirectory
                ->Entries[rollbackIndex]
                 .IsPresent()
              && (
                newPageDirectory
                  ->Entries[rollbackIndex]
                   .GetTableBase() !=
                _kernelPageDirectory
                  ->Entries[rollbackIndex]
                   .GetTableBase()
              )
            ) {
              _memoryAllocator->Free(
                MemoryBlock {
                  newPageDirectory
                    ->Entries[rollbackIndex]
                     .GetTableBase(),
                  PAGE_SIZE
                }
              );
            }
          }

          return false;
        }

        IA32PageTable* kernelTable = reinterpret_cast<IA32PageTable*>(
          _kernelPageDirectory
            ->Entries[pageDirectoryEntryIndex]
             .GetTableBase()
        );
        IA32PageTable* newTable = reinterpret_cast<IA32PageTable*>(
          tableBlock.Base
        );

        for (
          UInt32 entryIndex = 0;
          entryIndex < PAGE_TABLE_ENTRY_COUNT;
          ++entryIndex
        ) {
          newTable->Entries[entryIndex] = kernelTable->Entries[entryIndex];
        }

        // copy the PDE flags (including User bit)
        newPageDirectory->Entries[pageDirectoryEntryIndex]
          = _kernelPageDirectory->Entries[pageDirectoryEntryIndex];

        // then replace the table base with the cloned table's address
        newPageDirectory
          ->Entries[pageDirectoryEntryIndex]
           .SetTableBase(tableBlock.Base);
      }

      // set up the recursive mapping slot for the new page directory
      // this allows the new address space to manipulate its own page tables
      // via the recursive mapping technique.
      UIntPtr newPageDirectoryPhysical = _cpu->VirtualToPhysical(
        reinterpret_cast<UIntPtr>(newPageDirectory)
      );

      newPageDirectory->Entries[PAGE_TABLE_RECURSIVE_SLOT].SetTableBase(
        newPageDirectoryPhysical
      );
      newPageDirectory->Entries[PAGE_TABLE_RECURSIVE_SLOT].SetPresent(true);
      newPageDirectory->Entries[PAGE_TABLE_RECURSIVE_SLOT].SetWritable(true);

      return true;
    } else {
      return false;
    }
  }
}
