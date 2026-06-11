/**
 * @file Include/Quantum/Servers/Run/RunServerConstants.hpp
 * @brief Declares constants for @ref @QRunSrv.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI.hpp>

namespace Quantum::Servers::Run {
  /**
   * @brief ABI version for the run server protocol.
   */
  constexpr UInt32 RunServerABIVersion = 1;

  /**
   * @brief IPC port ID for the run server.
   */
  constexpr IPCPortID RunServerPortID = 2;

  /**
   * @brief Maximum length of a working directory path, including the null
   *        terminator.
   */
  constexpr Size MaxWorkingDirectoryLength = 256;
}
