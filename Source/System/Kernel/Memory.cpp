/**
 * @file System/Kernel/Memory.cpp
 * @brief Kernel memory management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Arch/Memory.hpp"
#include "Heap.hpp"
#include "Logger.hpp"
#include "Memory.hpp"

namespace Quantum::System::Kernel {
  using LogLevel = Kernel::Logger::Level;

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    Arch::Memory::InitializePaging(bootInfoPhysicalAddress);

    UInt32 heapPageSize = Heap::GetPageSize();
    UInt64 totalBytes
      = static_cast<UInt64>(Arch::Memory::GetPhysicalAllocatorTotalPages())
      * heapPageSize;
    UInt64 usedBytes
      = static_cast<UInt64>(Arch::Memory::GetPhysicalAllocatorUsedPages())
      * heapPageSize;
    UInt64 freeBytes
      = static_cast<UInt64>(Arch::Memory::GetPhysicalAllocatorFreePages())
      * heapPageSize;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Physical allocator: pages total=%p used=%p free=%p bytes total=%p "
        "used=%p free=%p",
      Arch::Memory::GetPhysicalAllocatorTotalPages(),
      Arch::Memory::GetPhysicalAllocatorUsedPages(),
      Arch::Memory::GetPhysicalAllocatorFreePages(),
      totalBytes,
      usedBytes,
      freeBytes
    );
    Heap::DumpState();
  }

  void* Memory::AllocatePage(bool zero) {
    return Arch::Memory::AllocatePage(zero);
  }

  UInt32 Memory::GetKernelPageDirectoryPhysical() {
    return Arch::Memory::GetKernelPageDirectoryPhysical();
  }

  void Memory::MapPage(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    Arch::Memory::MapPage(
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  UInt32 Memory::CreateAddressSpace() {
    return Arch::Memory::CreateAddressSpace();
  }

  void Memory::DestroyAddressSpace(UInt32 pageDirectoryPhysical) {
    Arch::Memory::DestroyAddressSpace(pageDirectoryPhysical);
  }

  void Memory::MapPageInAddressSpace(
    UInt32 pageDirectoryPhysical,
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    Arch::Memory::MapPageInAddressSpace(
      pageDirectoryPhysical,
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  void Memory::ActivateAddressSpace(UInt32 pageDirectoryPhysical) {
    Arch::Memory::ActivateAddressSpace(pageDirectoryPhysical);
  }

  void Memory::FreePage(void* page) {
    Arch::Memory::FreePage(page);
  }
}
