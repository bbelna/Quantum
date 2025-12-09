//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Memory.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 paging and basic memory helpers.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  /**
   * Initializes paging with identity mappings based on the boot memory map.
   * @param bootInfoPhysicalAddress Physical address of boot info provided by
   * bootloader.
   */
  void InitializePaging(uint32 bootInfoPhysicalAddress);

  /**
   * Allocates a 4 KB physical page (identity mapped).
   * @return Pointer to the allocated page.
   */
  void* AllocatePage();

  /**
   * Frees a 4 KB physical page previously allocated.
   * @param page Pointer to the physical page to free.
   */
  void FreePage(void* page);

  /**
   * Maps a virtual page to a physical page with present/RW bits set.
   * Assumes identity mapping for page tables themselves.
   * @param virtualAddress Virtual address of the page to map.
   * @param physicalAddress Physical address of the page to map.
   * @param writable Whether the page should be writable.
   */
  void MapPage(
    uint32 virtualAddress,
    uint32 physicalAddress,
    bool writable = true
  );

  /**
   * Unmaps a virtual page (does not free the physical page).
   * @param virtualAddress Virtual address of the page to unmap.
   */
  void UnmapPage(uint32 virtualAddress);
}
