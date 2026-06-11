/**
 * @file Kernel/Memory/FreeHeapBlock.hpp
 * @brief Declares @ref @QKrnl::Memory::FreeHeapBlock.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Represents a free block of heap memory.
   */
  struct FreeHeapBlock {
    /**
     * @brief Size of the free block in bytes.
     */
    Size SizeInBytes;

    /**
     * @brief Pointer to the next free block in the linked list.
     */
    FreeHeapBlock* Next;
  };
}
