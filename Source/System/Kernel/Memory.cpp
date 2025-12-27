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
#include "PhysicalAllocator.hpp"

namespace Quantum::System::Kernel {
  using LogLevel = Kernel::Logger::Level;

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    Arch::Memory::InitializePaging(bootInfoPhysicalAddress);

    UInt64 totalBytes
      = static_cast<UInt64>(PhysicalAllocator::GetTotalPages())
      * PhysicalAllocator::pageSize;
    UInt64 usedBytes
      = static_cast<UInt64>(PhysicalAllocator::GetUsedPages())
      * PhysicalAllocator::pageSize;
    UInt64 freeBytes
      = static_cast<UInt64>(PhysicalAllocator::GetFreePages())
      * PhysicalAllocator::pageSize;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Physical allocator: pages total=%p used=%p free=%p bytes total=%p "
        "used=%p free=%p",
      PhysicalAllocator::GetTotalPages(),
      PhysicalAllocator::GetUsedPages(),
      PhysicalAllocator::GetFreePages(),
      totalBytes,
      usedBytes,
      freeBytes
    );
    Heap::DumpState();
  }

}
