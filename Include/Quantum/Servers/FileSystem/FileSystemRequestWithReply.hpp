/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemRequestWithReply.hpp
 * @brief Declares @ref @QFSABI::FileSystemRequestWithReply.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI.hpp>

#include "FileSystemOperation.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Base request type for file system operations that expect a reply.
   */
  using FileSystemRequestWithReply = ABIRequestWithReplyPort<
    FileSystemOperation
  >;
}
