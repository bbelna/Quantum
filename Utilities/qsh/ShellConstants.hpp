/**
 * @file Utilities/qsh/ShellTypes.hpp
 * @brief Declares constants for @ref @Q::Shell.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Clients/FileSystemClient.hpp>

#include "ShellTypes.hpp"

namespace Quantum::Shell {
  /**
   * @brief Maximum length of the command line input buffer.
   */
  constexpr Size MaxInputLength = 256;

  /**
   * @brief Maximum number of arguments parsed from a command line.
   */
  constexpr Size MaxArguments = 32;

  /**
   * @brief Maximum number of directory entries returned by `ls`.
   */
  constexpr Size MaxDirectoryEntries = 64;

  /**
   * @brief Maximum length of a volume label.
   */
  constexpr Size MaxVolumeLabelLength = FileSystemClient::MaxVolumeLabelLength;

  /**
   * @brief Maximum length of a file system path.
   */
  constexpr Size MaxPathLength = FileSystemClient::MaxPathLength;

  /**
   * @brief Filename of the Shell configuration file. The Shell looks
   *        for it in its current working directory at startup.
   */
  constexpr const char* ConfigurationFileName = "qsh.cfg";

  /**
   * @brief Configuration key whose value supplies the search directory
   *        used by the Shell when resolving bare command names.
   */
  constexpr const char* ConfigurationPathKey = "PATH";

  /**
   * @brief Maximum number of bytes read from @ref ConfigurationFileName.
   */
  constexpr Size MaxConfigurationFileSize = 4096;
}
