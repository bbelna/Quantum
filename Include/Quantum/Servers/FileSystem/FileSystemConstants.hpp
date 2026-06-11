/**
 * @file Include/Quantum/Servers/FileSystem/Core/FileSystemConstants.hpp
 * @brief Declares file system server ABI constants.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/ABI.hpp>

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief ABI version for the file system server protocol.
   */
  constexpr UInt32 FileSystemABIVersion = 1;

  /**
   * @brief IPC port ID for the file system server.
   */
  constexpr IPCPortID FileSystemPortID = 6;

  /**
   * @brief Maximum length of a volume ID, including null terminator.
   *
   * Volume IDs are short, unique, programmatic identifiers assigned at
   * mount time (e.g., `"FDC0"`, `"STRP"`). All internal path resolution
   * and API calls use the Volume ID; the human-readable label is for
   * display only.
   */
  constexpr Size FileSystemMaxVolumeIDLength = 8;

  /**
   * @brief Maximum length of a volume label, including null terminator.
   */
  constexpr Size FileSystemMaxVolumeLabelLength = 32;

  /**
   * @brief Maximum length of a file path, including null terminator.
   */
  constexpr Size FileSystemMaxPathLength = 256;

  /**
   * @brief Maximum length of a file or directory name, including null
   *        terminator.
   */
  constexpr Size FileSystemMaxFileNameLength = 128;
}
