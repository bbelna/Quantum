/**
 * @file Servers/Run/Core/ELF32/ELF32ProgramHeader.hpp
 * @brief Declares @ref @QRunSrv::ELF32ProgramHeader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

#include <ELF/ELFSegmentFlags.hpp>
#include <ELF/ELFSegmentType.hpp>

namespace Quantum::Servers::Run::Core::ELF32 {
  /**
   * @brief ELF32 program header.
   */
  struct ELF32ProgramHeader {
    /**
     * @brief Segment type (e.g., loadable, dynamic linking info, etc.).
     */
    ELFSegmentType SegmentType;

    /**
     * @brief Segment flags (e.g., executable, writable, readable).
     */
    UInt32 FileOffset;

    /**
     * @brief Virtual address of the segment in memory.
     */
    UInt32 VirtualAddress;

    /**
     * @brief Physical address of the segment (ignored on many platforms).
     */
    UInt32 PhysicalAddress;

    /**
     * @brief Size of the segment in the file.
     */
    UInt32 FileSizeInBytes;

    /**
     * @brief Size of the segment in memory (can be larger than file size for
     *        segments with uninitialized data).
     */
    UInt32 MemorySizeInBytes;

    /**
     * @brief Segment flags (e.g., executable, writable, readable).
     */
    ELFSegmentFlags SegmentFlags;

    /**
     * @brief Segment alignment in memory and file (must be a power of 2).
     */
    UInt32 Alignment;
  };
}
