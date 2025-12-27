/**
 * @file System/Kernel/Arch/IA32/Paging.cpp
 * @brief IA32 paging support.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Align.hpp>
#include <Types.hpp>

#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/LinkerSymbols.hpp"
#include "Arch/IA32/Paging.hpp"
#include "Arch/IA32/PhysicalAllocator.hpp"
#include "Heap.hpp"
#include "Logger.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using ::Quantum::AlignUp;

  using LogLevel = Kernel::Logger::Level;

  alignas(Paging::_pageSize)
  UInt32 Paging::_pageDirectory[Paging::_pageDirectoryEntries];

  alignas(Paging::_pageSize)
  UInt32 Paging::_firstPageTable[Paging::_pageTableEntries];

  UInt32* Paging::GetPageDirectoryVirtual() {
    return reinterpret_cast<UInt32*>(_recursivePageDirectory);
  }

  UInt32* Paging::GetPageTableVirtual(UInt32 pageDirectoryIndex) {
    return reinterpret_cast<UInt32*>(
      _recursivePageTablesBase + pageDirectoryIndex * _pageSize
    );
  }

  UInt32* Paging::EnsurePageTable(UInt32 pageDirectoryIndex) {
    if (_pageDirectory[pageDirectoryIndex] & pagePresent) {
      // identity map keeps tables reachable even before higher-half switch
      return reinterpret_cast<UInt32*>(
        _pageDirectory[pageDirectoryIndex] & ~0xFFF
      );
    }

    UInt32 tablePhysical = 0;
    UInt32* table = nullptr;

    if (pageDirectoryIndex == 0) {
      // reuse the kernel's first page table (in .bss) for the first 4 MB
      table = _firstPageTable;
      tablePhysical = PhysicalAllocator::KernelVirtualToPhysical(
        reinterpret_cast<UInt32>(_firstPageTable)
      );
    } else {
      tablePhysical = PhysicalAllocator::AllocatePage(true);
      table = reinterpret_cast<UInt32*>(tablePhysical);

      for (UInt32 i = 0; i < _pageTableEntries; ++i) {
        table[i] = 0;
      }
    }

    _pageDirectory[pageDirectoryIndex]
      = tablePhysical
      | pagePresent
      | pageWrite;

    // return using the identity-mapped address
    // (physical == virtual in the low window).
    return reinterpret_cast<UInt32*>(tablePhysical);
  }

  void Paging::EnsureKernelHeapTables() {
    constexpr UInt32 heapBase = Paging::kernelHeapBase;
    constexpr UInt32 heapBytes = Paging::kernelHeapBytes;
    UInt32 startIndex = heapBase >> 22;
    UInt32 endIndex = (heapBase + heapBytes - 1) >> 22;

    for (UInt32 index = startIndex; index <= endIndex; ++index) {
      EnsurePageTable(index);
    }
  }

  void Paging::Initialize(UInt32 bootInfoPhysicalAddress) {
    PhysicalAllocator::Initialize(bootInfoPhysicalAddress);

    // clear directory and first table
    for (int i = 0; i < static_cast<int>(_pageDirectoryEntries); ++i) {
      _pageDirectory[i] = 0;
      _firstPageTable[i] = 0;
    }

    // identity map managedBytes (keep identity window for now)
    UInt32 managedBytes = PhysicalAllocator::GetManagedBytes();
    UInt32 tablesNeeded
      = (managedBytes + (4 * 1024 * 1024 - 1)) / (4 * 1024 * 1024);

    if (tablesNeeded > 1024) {
      tablesNeeded = 1024;
    }

    for (UInt32 tableIndex = 0; tableIndex < tablesNeeded; ++tableIndex) {
      UInt32* table = EnsurePageTable(tableIndex);

      UInt32 base = tableIndex * _pageTableEntries * _pageSize;

      for (UInt32 i = 0; i < _pageTableEntries; ++i) {
        table[i]
          = (base + i * _pageSize) | pagePresent | pageWrite | pageGlobal;
      }

      if (tableIndex == 0) {
        table[0] = 0; // guard null page
      }

      _pageDirectory[tableIndex]
        = reinterpret_cast<UInt32>(table) | pagePresent | pageWrite;
    }

    // map the kernel image into the higher half
    UInt32 kernelPhysicalStart = reinterpret_cast<UInt32>(&__phys_start);
    UInt32 kernelPhysicalEnd = reinterpret_cast<UInt32>(&__phys_end);
    UInt32 kernelSizeBytes = kernelPhysicalEnd - kernelPhysicalStart;

    for (UInt32 offset = 0; offset < kernelSizeBytes; offset += _pageSize) {
      UInt32 physicalAddress = kernelPhysicalStart + offset;
      UInt32 virtualAddress = Paging::kernelVirtualBase + offset;

      MapPage(virtualAddress, physicalAddress, true, false, true);
    }

    EnsureKernelHeapTables();

    // install recursive mapping in the last PDE
    UInt32 pageDirectoryPhysical = PhysicalAllocator::KernelVirtualToPhysical(
      reinterpret_cast<UInt32>(_pageDirectory)
    );

    _pageDirectory[Paging::recursiveSlot]
      = pageDirectoryPhysical | pagePresent | pageWrite;

    // load directory and enable paging,
    // invalidate the null page TLB entry after enabling
    CPU::LoadPageDirectory(pageDirectoryPhysical);
    CPU::EnablePaging();
    CPU::InvalidatePage(0);
  }

  void Paging::MapPage(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32* table = EnsurePageTable(pageDirectoryIndex);
    UInt32 flags = pagePresent
      | (writable ? pageWrite : 0)
      | (user ? pageUser : 0)
      | (global ? pageGlobal : 0);

    table[pageTableIndex] = (physicalAddress & ~0xFFF) | flags;

    if (user) {
      _pageDirectory[pageDirectoryIndex] |= pageUser;
    }

    CPU::InvalidatePage(virtualAddress);
  }

  void Paging::UnmapPage(UInt32 virtualAddress) {
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

    if (!(_pageDirectory[pageDirectoryIndex] & pagePresent)) {
      return;
    }

    UInt32* table = reinterpret_cast<UInt32*>(
      _pageDirectory[pageDirectoryIndex] & ~0xFFF
    );

    table[pageTableIndex] = 0;

    CPU::InvalidatePage(virtualAddress);
  }

  void Paging::MapRange(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    UInt32 lengthBytes,
    bool writable,
    bool user,
    bool global
  ) {
    UInt32 bytes = AlignUp(lengthBytes, _pageSize);
    UInt32 count = bytes / _pageSize;

    for (UInt32 i = 0; i < count; ++i) {
      MapPage(
        virtualAddress + i * _pageSize,
        physicalAddress + i * _pageSize,
        writable,
        user,
        global
      );
    }
  }

  void Paging::UnmapRange(UInt32 virtualAddress, UInt32 lengthBytes) {
    UInt32 bytes = AlignUp(lengthBytes, _pageSize);
    UInt32 count = bytes / _pageSize;

    for (UInt32 i = 0; i < count; ++i) {
      UnmapPage(virtualAddress + i * _pageSize);
    }
  }

  UInt32 Paging::GetPageDirectoryEntry(UInt32 virtualAddress) {
    UInt32 index = (virtualAddress >> 22) & 0x3FF;
    UInt32* directory = GetPageDirectoryVirtual();

    return directory[index];
  }

  UInt32 Paging::GetPageTableEntry(UInt32 virtualAddress) {
    UInt32 directoryEntry = GetPageDirectoryEntry(virtualAddress);

    if ((directoryEntry & pagePresent) == 0) {
      return 0;
    }

    UInt32 tableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32* table = GetPageTableVirtual((virtualAddress >> 22) & 0x3FF);

    return table[tableIndex];
  }

  UInt32 Paging::GetKernelPageDirectoryPhysical() {
    return PhysicalAllocator::KernelVirtualToPhysical(
      reinterpret_cast<UInt32>(_pageDirectory)
    );
  }

  const UInt32* Paging::GetKernelPageDirectoryEntries() {
    return _pageDirectory;
  }

  bool Paging::HandlePageFault(
    const Interrupts::Context& context,
    UInt32 faultAddress,
    UInt32 errorCode
  ) {
    CString accessType = (errorCode & 0x2) ? "write" : "read";
    CString mode = (errorCode & 0x4) ? "user" : "kernel";
    bool presentViolation = (errorCode & 0x1) != 0;
    bool reservedBit = (errorCode & 0x8) != 0;
    bool instructionFetch = (errorCode & 0x10) != 0;

    UInt32 pde = GetPageDirectoryEntry(faultAddress);
    UInt32 pte = GetPageTableEntry(faultAddress);

    Logger::Write(LogLevel::Error, ":( PAGE FAULT");
    Logger::WriteFormatted(
      LogLevel::Error,
      "  addr=%p (%s %s) err=%p present=%s reserved=%s instr=%s",
      faultAddress,
      accessType,
      mode,
      errorCode,
      presentViolation ? "yes" : "no",
      reservedBit ? "yes" : "no",
      instructionFetch ? "yes" : "no"
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  EIP=%p ESP=%p CR2=%p PDE=%p PTE=%p",
      context.eip,
      context.esp,
      faultAddress,
      pde,
      pte
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  EAX=%p EBX=%p ECX=%p EDX=%p",
      context.eax,
      context.ebx,
      context.ecx,
      context.edx
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  ESI=%p EDI=%p EBP=%p",
      context.esi,
      context.edi,
      context.ebp
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  Task=%u coordinator=%s",
      Kernel::Task::GetCurrentId(),
      Kernel::Task::IsCurrentTaskCoordinator() ? "yes" : "no"
    );
    if ((errorCode & 0x4) != 0) {
      const UInt32* frame = reinterpret_cast<const UInt32*>(&context);
      UInt32 userEsp = frame[13];
      UInt32 userSs = frame[14];

      Logger::WriteFormatted(
        LogLevel::Error,
        "  User ESP=%p SS=%p",
        userEsp,
        userSs
      );
    }

    // stub: escalate for now; future VM/pager can service demand faults
    return false;
  }
}
