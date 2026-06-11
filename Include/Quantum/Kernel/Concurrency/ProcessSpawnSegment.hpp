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

#include <Quantum/Core.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Describes a single loadable segment within a spawn binary buffer.
   *        Used to communicate per-segment memory permissions from the ELF
   *        loader to the kernel.
   */
  struct ProcessSpawnSegment {
    /**
     * @brief Byte offset of this segment from @ref ProcessSpawnParameters
     *        @ref ProcessSpawnParameters::SourceBase.
     */
    Size OffsetInBytes;

    /**
     * @brief Size of this segment in bytes.
     */
    Size SizeInBytes;

    /**
     * @brief Permission flags for this segment.
     *        Bit 0 = Read, Bit 1 = Write, Bit 2 = Execute.
     */
    UInt32 Permissions;
  };
}
