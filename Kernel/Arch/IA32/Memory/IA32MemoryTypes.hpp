/**
 * @file Kernel/Arch/IA32/Memory/IA32MemoryTypes.hpp
 * @brief Declares @QKrnlIA32::Memory types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Arch::IA32::Memory {
  struct E820BootInfo;
  struct IA32GDTEntry;
  struct IA32PageDirectoryEntry;
  struct IA32PageTableEntry;
}

using namespace Quantum::Kernel::Arch::IA32::Memory;
