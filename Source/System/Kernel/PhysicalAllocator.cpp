/**
 * @file System/Kernel/PhysicalAllocator.cpp
 * @brief Physical page allocator wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/Memory.hpp"
#include "PhysicalAllocator.hpp"

namespace Quantum::System::Kernel {
  void* PhysicalAllocator::AllocatePage(bool zero) {
    return Arch::Memory::AllocatePage(zero);
  }

  void PhysicalAllocator::FreePage(void* page) {
    Arch::Memory::FreePage(page);
  }

  UInt32 PhysicalAllocator::GetTotalPages() {
    return Arch::Memory::GetPhysicalAllocatorTotalPages();
  }

  UInt32 PhysicalAllocator::GetUsedPages() {
    return Arch::Memory::GetPhysicalAllocatorUsedPages();
  }

  UInt32 PhysicalAllocator::GetFreePages() {
    return Arch::Memory::GetPhysicalAllocatorFreePages();
  }
}
