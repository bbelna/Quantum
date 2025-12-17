/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Memory/AlignedMetadata.hpp
 * Metadata stored immediately before an aligned payload.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/Memory/FreeBlock.hpp>

namespace Quantum::System::Kernel::Types::Memory {
  /**
   * Metadata stored immediately before an aligned payload.
   */
  struct AlignedMetadata {
    /**
     * Alignment marker to detect metadata.
     */
    UInt32 magic;

    /**
     * Owning free-block header for the allocation.
     */
    FreeBlock* block;

    /**
     * Offset from the start of the block payload to the aligned address.
     */
    UInt32 payloadOffset;
  };
}
