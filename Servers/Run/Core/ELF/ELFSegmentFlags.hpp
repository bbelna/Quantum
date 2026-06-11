/**
 * @file Servers/Run/Core/ELF/ELFSegmentFlags.hpp
 * @brief Declares @ref @QRunSrv::Core::ELF::ELFSegmentFlags.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <RunServerTypes.hpp>

namespace Quantum::Servers::Run::Core::ELF {
  /**
   * @brief ELF program header segment flags.
   */
  enum class ELFSegmentFlags : UInt32 {
    /**
     * @brief No flags set.
     */
    None = 0,

    /**
     * @brief Executable segment.
     */
    Execute = 1u << 0,

    /**
     * @brief Writable segment.
     */
    Write = 1u << 1,

    /**
     * @brief Readable segment.
     */
    Read = 1u << 2
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Servers::Run::Core::ELF,
  ELFSegmentFlags
)
