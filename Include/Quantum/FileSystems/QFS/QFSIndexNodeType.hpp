/**
 * @file Include/Quantum/FileSystems/QFS/QFSIndexNodeType.hpp
 * @brief Declares @ref QFSIndexNodeType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::QFS {
  /**
   * @brief Index node file type values (stored in bits 15..12 of
   *        @ref QFSIndexNode::Mode).
   */
  enum class QFSIndexNodeType : UInt16 {
    /**
     * @brief Regular file.
     */
    RegularFile = 0x1000,

    /**
     * @brief Directory.
     */
    Directory = 0x2000,

    /**
     * @brief Symbolic link.
     */
    SymbolicLink = 0x3000,

    /**
     * @brief Bitmask for extracting the type from
     *        @ref QFSIndexNode::Mode.
     */
    TypeMask = 0xF000,

    /**
     * @brief Bitmask for extracting the permission bits from
     *        @ref QFSIndexNode::Mode.
     */
    PermissionMask = 0x0FFF
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::FileSystems::QFS, QFSIndexNodeType
)
