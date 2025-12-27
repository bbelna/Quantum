/**
 * @file System/Kernel/Arch/IA32/AddressSpace.cpp
 * @brief IA32 address space management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Arch/IA32/AddressSpace.hpp"
#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Paging.hpp"
#include "Arch/IA32/PhysicalAllocator.hpp"
#include "Panic.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  UInt32 AddressSpace::Create() {
    UInt32 directoryPhysical = PhysicalAllocator::AllocatePage(true);

    if (directoryPhysical == 0) {
      return 0;
    }

    UInt32* directory = reinterpret_cast<UInt32*>(directoryPhysical);
    UInt32 kernelStartIndex = Paging::kernelVirtualBase >> 22;
    const UInt32* kernelDirectory = Paging::GetKernelPageDirectoryEntries();

    for (UInt32 i = 0; i < 1024; ++i) {
      directory[i] = 0;
    }

    for (UInt32 i = kernelStartIndex; i < Paging::recursiveSlot; ++i) {
      directory[i] = kernelDirectory[i];
    }

    for (UInt32 i = 0; i < kernelStartIndex; ++i) {
      UInt32 entry = kernelDirectory[i];

      if ((entry & Paging::pagePresent) == 0) {
        continue;
      }

      UInt32 sourceTablePhysical = entry & ~0xFFFu;
      UInt32* sourceTable
        = reinterpret_cast<UInt32*>(sourceTablePhysical);
      UInt32 destTablePhysical = PhysicalAllocator::AllocatePage(true);

      if (destTablePhysical == 0) {
        PANIC("Failed to allocate page table");
      }

      UInt32* destTable = reinterpret_cast<UInt32*>(destTablePhysical);

      for (UInt32 j = 0; j < 1024; ++j) {
        destTable[j] = sourceTable[j];
      }

      directory[i] = (destTablePhysical & ~0xFFFu) | (entry & 0xFFFu);
    }

    directory[Paging::recursiveSlot]
      = directoryPhysical | Paging::pagePresent | Paging::pageWrite;

    return directoryPhysical;
  }

  void AddressSpace::Destroy(UInt32 pageDirectoryPhysical) {
    UInt32 kernelDirectory = Paging::GetKernelPageDirectoryPhysical();

    if (
      pageDirectoryPhysical == 0
      || pageDirectoryPhysical == kernelDirectory
    ) {
      return;
    }

    UInt32* directory = reinterpret_cast<UInt32*>(pageDirectoryPhysical);
    UInt32 kernelStartIndex = Paging::kernelVirtualBase >> 22;

    for (UInt32 i = 0; i < kernelStartIndex; ++i) {
      UInt32 entry = directory[i];

      if ((entry & Paging::pagePresent) == 0) {
        continue;
      }

      UInt32 tablePhysical = entry & ~0xFFFu;
      UInt32* table = reinterpret_cast<UInt32*>(tablePhysical);

      for (UInt32 j = 0; j < 1024; ++j) {
        UInt32 page = table[j];

        if ((page & Paging::pagePresent) == 0) {
          continue;
        }

        if ((page & Paging::pageGlobal) != 0) {
          continue;
        }

        UInt32 physical = page & ~0xFFFu;

        if (physical != 0) {
          PhysicalAllocator::FreePage(physical);
        }
      }

      PhysicalAllocator::FreePage(tablePhysical);
    }

    PhysicalAllocator::FreePage(pageDirectoryPhysical);
  }

  void AddressSpace::MapPage(
    UInt32 pageDirectoryPhysical,
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    if (pageDirectoryPhysical == 0) {
      return;
    }

    UInt32* directory = reinterpret_cast<UInt32*>(pageDirectoryPhysical);
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32 entry = directory[pageDirectoryIndex];
    UInt32* table = nullptr;

    if ((entry & Paging::pagePresent) != 0) {
      table = reinterpret_cast<UInt32*>(entry & ~0xFFFu);
    } else {
      UInt32 tablePhysical = PhysicalAllocator::AllocatePage(true);

      if (tablePhysical == 0) {
        PANIC("Failed to allocate page table");
      }

      table = reinterpret_cast<UInt32*>(tablePhysical);
      directory[pageDirectoryIndex]
        = (tablePhysical & ~0xFFFu) | Paging::pagePresent | Paging::pageWrite;
    }

    UInt32 flags = Paging::pagePresent
      | (writable ? Paging::pageWrite : 0)
      | (user ? Paging::pageUser : 0)
      | (global ? Paging::pageGlobal : 0);

    table[pageTableIndex] = (physicalAddress & ~0xFFFu) | flags;

    if (user) {
      directory[pageDirectoryIndex] |= Paging::pageUser;
    }

    if (pageDirectoryPhysical == Paging::GetKernelPageDirectoryPhysical()) {
      CPU::InvalidatePage(virtualAddress);
    }
  }

  void AddressSpace::Activate(UInt32 pageDirectoryPhysical) {
    if (pageDirectoryPhysical == 0) {
      return;
    }

    CPU::LoadPageDirectory(pageDirectoryPhysical);
  }
}
