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
   * Initializes paging with an identity map for the low 4 MB and installs
   * the initial page directory.
   */
  void InitializePaging();

  /**
   * Allocates a 4 KB physical page (identity mapped).
   */
  void* AllocatePage();

  /**
   * Maps a virtual page to a physical page with present/RW bits set.
   * Assumes identity mapping for page tables themselves.
   */
  void MapPage(uint32 virtualAddr, uint32 physicalAddr, bool writable = true);
}
