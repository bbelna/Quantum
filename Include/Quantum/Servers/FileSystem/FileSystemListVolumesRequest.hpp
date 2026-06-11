/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemListVolumesRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemListVolumesRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemResult.hpp"
#include "FileSystemRequestWithReply.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Request structure for the `ListVolumes` operation.
   */
  struct FileSystemListVolumesRequest : public FileSystemRequestWithReply {};
}
