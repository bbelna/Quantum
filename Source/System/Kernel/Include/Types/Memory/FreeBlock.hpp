/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Memory/FreeBlock.hpp
 * Describes a free block of memory used in the memory management system.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Types::Memory {
  /**
   * Represents a free block of memory.
   */
  struct FreeBlock {
    /**
     * Size of the free block in bytes.
     */
    UInt32 size;

    /**
     * Pointer to the next free block in the linked list.
     */
    FreeBlock* next;
  };
}
