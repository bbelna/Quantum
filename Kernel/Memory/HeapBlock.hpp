/**
 * @file Kernel/Memory/HeapBlock.hpp
 * @brief Declares @ref @QKrnl::Memory::HeapBlock.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Block of addresses in a specific address space's heap.
   */
  struct HeapBlock : public MemoryBlock {};
}
