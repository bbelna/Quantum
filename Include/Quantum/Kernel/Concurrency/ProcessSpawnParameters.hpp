/**
 * @file Include/Quantum/Kernel/Concurrency/ProcessSpawnParameters.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ProcessSpawnParameters.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Kernel/Memory/MemoryTypes.hpp>

#include "ProcessSpawnSegment.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Maximum number of inherited streams per process.
   */
  inline constexpr Size MaxInheritedStreams = 3;

  /**
   * @brief Parameters for spawning a new process with binary mapping.
   */
  struct ProcessSpawnParameters {
    /**
     * @brief Source virtual address of the binary data in the caller's
     *        address space.
     */
    UIntPtr SourceBase;

    /**
     * @brief Size of the binary image in bytes.
     */
    Size SizeInBytes;

    /**
     * @brief Target virtual address in the child's address space where the
     *        binary should be mapped. When zero, the kernel uses the
     *        page-aligned base of `entryPoint` instead.
     */
    UIntPtr TargetBase;

    /**
     * @brief Number of segment descriptors in the @ref Segments array. When
     *        zero, all pages are mapped with read, write, and user permissions.
     */
    Size SegmentCount;

    /**
     * @brief Pointer to an array of `SpawnSegment` descriptors in the
     *        caller's address space, or `nullptr` if `SegmentCount` is zero.
     */
    const ProcessSpawnSegment* Segments;

    /**
     * @brief Number of command-line arguments.
     */
    Size ArgumentCount = 0;

    /**
     * @brief Pointer to packed null-terminated argument strings in the
     *        caller's address space. The strings are concatenated with a
     *        `\0` separator between each. For example, two arguments
     *        `"ls"` and `"-la"` are stored as `"ls\0-la\0"`.
     */
    const char* ArgumentData = nullptr;

    /**
     * @brief Total size in bytes of the packed argument data, including
     *        all null terminators.
     */
    Size ArgumentDataSize = 0;

    /**
     * @brief Number of inherited stream buffer IDs (0 to 3). When non-zero,
     *        the kernel writes the buffer IDs into the child's process
     *        stream table at a fixed virtual address.
     */
    UInt8 StreamCount = 0;

    /**
     * @brief Inherited stream buffer IDs. Index 0 = stdin, 1 = stdout,
     *        2 = stderr. A value of `0` means no stream for that index.
     */
    Memory::SharedBufferID StreamBufferIDs[MaxInheritedStreams] = {};
  };
}
